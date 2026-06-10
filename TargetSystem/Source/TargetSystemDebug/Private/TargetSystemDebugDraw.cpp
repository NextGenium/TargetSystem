// Copyright (c) 2024 NextGenium

#include "TargetSystemDebugDraw.h"

#if ENABLE_DRAW_DEBUG

#include "Containers/Ticker.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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
	TEXT("  2 = + lock-on points (green = free, red = blocked)\n")
	TEXT("  3 = + point tag labels"),
	ECVF_Cheat);

namespace
{
	FTSTicker::FDelegateHandle GTickerHandle;

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

		DrawDebugLine(World, From, To, FColor::Yellow, false, -1.f, 0, 2.f);
		DrawDebugSphere(World, To, 24.f, 12, FColor::Yellow, false, -1.f, 0, 1.f);

		if (Level < 2)
		{
			return;
		}

		const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(Target);
		if (!Interface)
		{
			return;
		}

		for (const UTargetPointComponent* Point : Interface->GetTargetPoints())
		{
			if (!IsValid(Point))
			{
				continue;
			}

			const bool bBlocked = !Point->StateTags.IsEmpty();
			const FColor Color = bBlocked ? FColor::Red : FColor::Green;
			const FVector PointLocation = Point->GetComponentLocation();

			DrawDebugSphere(World, PointLocation, 12.f, 8, Color, false, -1.f, 0, 1.f);

			if (Level >= 3 && !Point->PointTags.IsEmpty())
			{
				DrawDebugString(World, PointLocation, Point->PointTags.ToStringSimple(), nullptr, Color, 0.f, true);
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
