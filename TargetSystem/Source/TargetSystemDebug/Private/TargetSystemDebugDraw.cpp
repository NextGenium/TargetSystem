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
	TEXT("  1 = line player->locked point + anchor marker (through the mesh)\n")
	TEXT("  2 = + lock-on points with name/point-tag labels through the mesh (green = lockable, red = blocked)\n")
	TEXT("  3 = + blocker StateTags appended to the labels\n")
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
		const UTargetPointComponent* LockedPoint = Component->GetLockedPoint();

		const FVector From = IsValid(Owner) ? Owner->GetActorLocation() : Target->GetActorLocation();
		// Aim the lock trace at the actual locked POINT (head/leg/...) we are anchored to, not the
		// actor origin. Fall back to the actor location only when no point is locked.
		const FVector To = IsValid(LockedPoint) ? LockedPoint->GetComponentLocation() : Target->GetActorLocation();

		FVector CamRight, CamUp;
		GetCameraBasis(Owner, CamRight, CamUp);

		// SDPG_Foreground so the trace + anchor read through the target mesh (the locked point is
		// often inside the body).
		DrawDebugLine(World, From, To, FColor::Yellow, false, -1.f, SDPG_Foreground, 2.f);
		DrawDebugSphere(World, To, 24.f, 12, FColor::Yellow, false, -1.f, SDPG_Foreground, 1.f);

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
					// A lockable point is green; a runtime-blocked one (StateTags present) is red. The
					// active point shares the green but is drawn larger — the yellow trace marks which
					// one we are anchored to.
					const FColor Color = bBlocked ? FColor::Red : FColor::Green;
					const FVector PointLocation = Point->GetComponentLocation();

					// Camera-facing circle (flat marker), SDPG_Foreground so it reads through the mesh.
					DrawDebugCircle(World, PointLocation, bActive ? 16.f : 12.f, 16, Color,
						false, -1.f, SDPG_Foreground, 1.5f, CamRight, CamUp, false);

					// Label next to the marker: "Name [point tags]" (mirrors the MotionWarping point
					// visualizer). DrawDebugString is screen-space, so the text reads through the mesh
					// like the marker. Drawn with the points (level 2); level 3 appends the blocker tags.
					const FString Tags = Point->PointTags.IsEmpty()
						? TEXT("(no point tags)")
						: Point->PointTags.ToStringSimple();
					FString Label = FString::Printf(TEXT("%s [%s]"), *Point->GetName(), *Tags);
					if (bActive)
					{
						Label += TEXT(" [active]");
					}
					if (Level >= 3 && bBlocked)
					{
						Label += FString::Printf(TEXT(" [blocked: %s]"), *Point->StateTags.ToStringSimple());
					}
					DrawDebugString(World, PointLocation, Label, nullptr, Color, 0.f, true);
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

	// Shared with the Gameplay Debugger category: it calls this from CollectData so opening the
	// debugger (apostrophe) draws the exact same foreground (x-ray) trace + point markers + labels
	// as the cvar ticker, with no console command. Same SDPG_Foreground path => reads through the mesh.
	void DrawOverlay(UTargetLockComponent* Component, int32 Level)
	{
		DrawForComponent(Component, Level);
	}
}

#else // !ENABLE_DRAW_DEBUG

namespace TargetSystemDebugDraw
{
	void Register() {}
	void Unregister() {}
}

#endif
