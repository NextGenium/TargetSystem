// Copyright (c) 2024 NextGenium

#include "TargetSystemDebugDraw.h"

#if ENABLE_DRAW_DEBUG

#include "Containers/Ticker.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UObjectIterator.h"

#include "TargetPointComponent.h"
#include "TargetLockComponent.h"
#include "TargetSystemInterface.h"

static TAutoConsoleVariable<int32> CVarTargetSystemDebugDraw(
	TEXT("targetsystem.DebugDraw"),
	0,
	TEXT("Draw the target-lock overlay without the Gameplay Debugger.\n")
	TEXT("  0 = off\n")
	TEXT("  1 = line player->target + target marker\n")
	TEXT("  2 = + lock-on points (cyan = active, green = free, red = blocked)\n")
	TEXT("  3 = + point tag labels\n")
	TEXT("  4 = + candidate scores from the last lock-on request"),
	ECVF_Cheat);

namespace
{
	FTSTicker::FDelegateHandle GTickerHandle;

	// Resolve the player camera's right/up axes so the debug circles face the camera. Falls back
	// to world axes when there is no player camera manager.
	void GetCameraBasis(const AActor* Owner, FVector& OutRight, FVector& OutUp)
	{
		OutRight = FVector::RightVector;
		OutUp = FVector::UpVector;

		const APawn* Pawn = Cast<APawn>(Owner);
		const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
		if (PC && PC->PlayerCameraManager)
		{
			const FRotationMatrix CameraMatrix(PC->PlayerCameraManager->GetCameraRotation());
			OutRight = CameraMatrix.GetUnitAxis(EAxis::Y);
			OutUp = CameraMatrix.GetUnitAxis(EAxis::Z);
		}
	}

	void DrawForComponent(UTargetLockComponent* Component, int32 Level)
	{
		if (!IsValid(Component) || !Component->IsLocked())
		{
			return;
		}

		AActor* Target = Component->GetLockedOnTargetActor();
		if (!IsValid(Target))
		{
			return;
		}

		UWorld* World = Component->GetWorld();
		if (!World)
		{
			return;
		}

		const AActor* Owner = Component->GetOwner();
		const FVector From = IsValid(Owner) ? Owner->GetActorLocation() : Target->GetActorLocation();
		const FVector To = Target->GetActorLocation();

		FVector CamRight, CamUp;
		GetCameraBasis(Owner, CamRight, CamUp);

		DrawDebugLine(World, From, To, FColor::Yellow, false, -1.f, 0, 2.f);
		DrawDebugSphere(World, To, 24.f, 12, FColor::Yellow, false, -1.f, 0, 1.f);

		if (Level >= 2)
		{
			if (const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(Target))
			{
				const UTargetPointComponent* ActivePoint = Component->GetLockedPoint();

				for (const UTargetPointComponent* Point : Interface->GetTargetPoints())
				{
					if (!IsValid(Point))
					{
						continue;
					}

					const bool bActive = (Point == ActivePoint);
					const bool bBlocked = !Point->StateTags.IsEmpty();
					const FColor Color = bActive ? FColor::Cyan : (bBlocked ? FColor::Red : FColor::Green);
					const FVector PointLocation = Point->GetComponentLocation();

					// Camera-facing circle instead of a sphere so the point reads as a flat marker.
					DrawDebugCircle(World, PointLocation, bActive ? 16.f : 12.f, 16, Color,
						false, -1.f, 0, 1.5f, CamRight, CamUp, false);

					if (Level >= 3)
					{
						FString Label = Point->PointTags.IsEmpty() ? TEXT("(no tags)") : Point->PointTags.ToStringSimple();
						if (bActive)
						{
							Label += TEXT(" [active]");
						}
						DrawDebugString(World, PointLocation, Label, nullptr, Color, 0.f, true);
					}
				}
			}
		}

		if (Level >= 4)
		{
			// Candidate ranking from the last lock-on request: winner green, the rest orange.
			for (const FTargetLockDebugCandidate& Candidate : Component->GetDebugLockOnCandidates())
			{
				const AActor* Actor = Candidate.Actor.Get();
				if (!IsValid(Actor))
				{
					continue;
				}

				const FVector Location = Actor->GetActorLocation();
				const FColor Color = Candidate.bWinner ? FColor::Green : FColor::Orange;

				DrawDebugCircle(World, Location + FVector(0.f, 0.f, 20.f), 28.f, 16, Color,
					false, -1.f, 0, 2.f, CamRight, CamUp, false);
				DrawDebugString(World, Location + FVector(0.f, 0.f, 70.f),
					FString::Printf(TEXT("%.2f%s"), Candidate.Score, Candidate.bWinner ? TEXT(" WIN") : TEXT("")),
					nullptr, Color, 0.f, true);
			}
		}
	}

	bool Tick(float /*DeltaTime*/)
	{
		const int32 Level = CVarTargetSystemDebugDraw.GetValueOnGameThread();
		if (Level <= 0)
		{
			return true;
		}

		for (TObjectIterator<UTargetLockComponent> It; It; ++It)
		{
			UTargetLockComponent* Component = *It;
			if (!IsValid(Component))
			{
				continue;
			}

			const UWorld* World = Component->GetWorld();
			if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE))
			{
				continue;
			}

			DrawForComponent(Component, Level);
		}

		return true;
	}
}

namespace TargetSystemDebugDraw
{
	void Register()
	{
		if (!GTickerHandle.IsValid())
		{
			GTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&Tick));
		}
	}

	void Unregister()
	{
		if (GTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GTickerHandle);
			GTickerHandle.Reset();
		}
	}
}

#else // !ENABLE_DRAW_DEBUG

namespace TargetSystemDebugDraw
{
	void Register() {}
	void Unregister() {}
}

#endif
