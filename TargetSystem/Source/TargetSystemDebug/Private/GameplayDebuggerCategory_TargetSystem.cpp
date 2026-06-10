// Copyright (c) 2024 NextGenium

#include "GameplayDebuggerCategory_TargetSystem.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "TargetPointComponent.h"
#include "TargetSystemComponent.h"
#include "TargetSystemInterface.h"

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_TargetSystem::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_TargetSystem());
}

void FGameplayDebuggerCategory_TargetSystem::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	APawn* PlayerPawn = OwnerPC ? OwnerPC->GetPawn() : nullptr;
	if (!IsValid(PlayerPawn))
	{
		AddTextLine(TEXT("{red}No player pawn"));
		return;
	}

	UTargetSystemComponent* Component = PlayerPawn->FindComponentByClass<UTargetSystemComponent>();
	if (!Component)
	{
		AddTextLine(FString::Printf(TEXT("{red}'%s' has no UTargetSystemComponent"), *GetNameSafe(PlayerPawn)));
		return;
	}

	const bool bLocked = Component->IsLocked();
	AddTextLine(FString::Printf(TEXT("{white}Lock: %s"), bLocked ? TEXT("{green}LOCKED") : TEXT("{grey}none")));

	AActor* Target = Component->GetLockedOnTargetActor();
	if (!bLocked || !IsValid(Target))
	{
		return;
	}

	AddTextLine(FString::Printf(TEXT("{white}Target: {yellow}%s"), *GetNameSafe(Target)));

	// 3D line player -> target + marker on the target.
	const FVector From = PlayerPawn->GetActorLocation();
	const FVector To = Target->GetActorLocation();
	AddShape(FGameplayDebuggerShape::MakeSegment(From, To, 2.0f, FColor::Yellow));
	AddShape(FGameplayDebuggerShape::MakePoint(To, 12.0f, FColor::Yellow));

	const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(Target);
	if (!Interface)
	{
		AddTextLine(TEXT("{grey}Target has no ITargetSystemInterface (locked on actor location)"));
		return;
	}

	const TArray<UTargetPointComponent*> Points = Interface->GetTargetPoints();
	AddTextLine(FString::Printf(TEXT("{white}Points: {yellow}%d"), Points.Num()));

	for (const UTargetPointComponent* Point : Points)
	{
		if (!IsValid(Point))
		{
			continue;
		}

		// Green = no runtime blockers; red = has StateTags (e.g. Block.Stunned).
		const bool bBlocked = !Point->StateTags.IsEmpty();
		const FColor Color = bBlocked ? FColor::Red : FColor::Green;
		AddShape(FGameplayDebuggerShape::MakePoint(Point->GetComponentLocation(), 8.0f, Color));

		const FString PointTagsStr = Point->PointTags.IsEmpty() ? TEXT("(no tags)") : Point->PointTags.ToStringSimple();
		const FString StateStr = Point->StateTags.IsEmpty()
			? FString()
			: FString::Printf(TEXT(" {red}[%s]"), *Point->StateTags.ToStringSimple());

		AddTextLine(FString::Printf(TEXT("    %s{white}%s%s"),
			bBlocked ? TEXT("{red}● ") : TEXT("{green}● "),
			*PointTagsStr,
			*StateStr));
	}
}

#endif
