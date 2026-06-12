// Copyright (c) 2024 NextGenium

#include "GameplayDebuggerCategory_TargetSystem.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "TargetPointComponent.h"
#include "TargetLockComponent.h"
#include "TargetSystemInterface.h"
#include "Targeting/TargetLockContext.h"

namespace
{
	const TCHAR* ModeName(ETargetSwitchMode Mode)
	{
		switch (Mode)
		{
		case ETargetSwitchMode::LockOn:      return TEXT("LockOn");
		case ETargetSwitchMode::SwitchLeft:  return TEXT("SwitchLeft");
		case ETargetSwitchMode::SwitchRight: return TEXT("SwitchRight");
		case ETargetSwitchMode::SwitchPoint: return TEXT("SwitchPoint");
		default:                             return TEXT("?");
		}
	}
}

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

	UTargetLockComponent* Component = PlayerPawn->FindComponentByClass<UTargetLockComponent>();
	if (!Component)
	{
		AddTextLine(FString::Printf(TEXT("{red}'%s' has no UTargetLockComponent"), *GetNameSafe(PlayerPawn)));
		return;
	}

	// Camera basis for the raw angle/distance readout shared by both candidate sections.
	FVector ViewLocation = PlayerPawn->GetActorLocation();
	FVector ViewForward = PlayerPawn->GetActorForwardVector();
	if (OwnerPC->PlayerCameraManager)
	{
		ViewLocation = OwnerPC->PlayerCameraManager->GetCameraLocation();
		ViewForward = OwnerPC->PlayerCameraManager->GetCameraRotation().Vector();
	}

	// Geometric angle from the camera forward to a world point (degrees). This is model-independent
	// context next to the engine score — NOT a re-derivation of the sort task's weighted formula.
	const auto RawAngleDeg = [&ViewLocation, &ViewForward](const FVector& WorldLocation) -> float
	{
		const FVector ToTarget = (WorldLocation - ViewLocation).GetSafeNormal();
		const float Dot = FMath::Clamp(FVector::DotProduct(ViewForward.GetSafeNormal(), ToTarget), -1.f, 1.f);
		return FMath::RadiansToDegrees(FMath::Acos(Dot));
	};

	const bool bLocked = Component->IsLocked();
	AddTextLine(FString::Printf(TEXT("{white}Lock: %s    {white}Mode(last): {yellow}%s"),
		bLocked ? TEXT("{green}LOCKED") : TEXT("{grey}none"),
		ModeName(Component->GetDebugLastMode())));

	// Candidate scoring: list the last request's ranking with the winner highlighted. Shown even
	// when not currently locked, so a failed/empty pick is still inspectable.
	const FVector PawnLocation = PlayerPawn->GetActorLocation();
	const auto DrawCandidates = [this, &RawAngleDeg, &PawnLocation](const TCHAR* Title, const TArray<FTargetLockDebugCandidate>& Candidates)
	{
		if (Candidates.Num() == 0)
		{
			return;
		}

		AddTextLine(FString::Printf(TEXT("{white}%s ({yellow}%d{white}):"), Title, Candidates.Num()));

		int32 Index = 0;
		for (const FTargetLockDebugCandidate& Candidate : Candidates)
		{
			const AActor* Actor = Candidate.Actor.Get();
			if (!IsValid(Actor))
			{
				continue;
			}
			++Index;

			const FVector Location = Actor->GetActorLocation();
			const float Angle = RawAngleDeg(Location);
			const float Distance = FVector::Dist(PawnLocation, Location);

			AddTextLine(FString::Printf(TEXT("  %s#%d %s{white}  score %.2f  ang %.0f deg  dist %.0f%s"),
				Candidate.bWinner ? TEXT("{green}") : TEXT("{orange}"),
				Index, *GetNameSafe(Actor), Candidate.Score, Angle, Distance,
				Candidate.bWinner ? TEXT("  {green}<- WIN") : TEXT("")));

			// World marker: winner green, the rest orange. Lifted a little so it clears the feet.
			const FColor Color = Candidate.bWinner ? FColor::Green : FColor::Orange;
			AddShape(FGameplayDebuggerShape::MakePoint(Location + FVector(0.f, 0.f, 20.f), 14.0f, Color));
		}
	};

	DrawCandidates(TEXT("Lock-on candidates"), Component->GetDebugLockOnCandidates());
	DrawCandidates(TEXT("Switch candidates"), Component->GetDebugSwitchCandidates());

	AActor* Target = Component->GetLockedOnTargetActor();
	if (!bLocked || !IsValid(Target))
	{
		return;
	}

	const float TargetDistance = FVector::Dist(PawnLocation, Target->GetActorLocation());
	AddTextLine(FString::Printf(TEXT("{white}Target: {yellow}%s  {white}dist {yellow}%.0f {white}/ lose {grey}%.0f"),
		*GetNameSafe(Target), TargetDistance, Component->GetLoseTargetDistance()));

	// 3D line player -> target + marker on the target.
	AddShape(FGameplayDebuggerShape::MakeSegment(PawnLocation, Target->GetActorLocation(), 2.0f, FColor::Yellow));
	AddShape(FGameplayDebuggerShape::MakePoint(Target->GetActorLocation(), 12.0f, FColor::Yellow));

	const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(Target);
	if (!Interface)
	{
		AddTextLine(TEXT("{grey}Target has no ITargetSystemInterface (locked on actor location)"));
		return;
	}

	const UTargetPointComponent* ActivePoint = Component->GetLockedPoint();
	const TArray<UTargetPointComponent*> Points = Interface->GetTargetPoints();
	AddTextLine(FString::Printf(TEXT("{white}Points: {yellow}%d"), Points.Num()));

	for (const UTargetPointComponent* Point : Points)
	{
		if (!IsValid(Point))
		{
			continue;
		}

		// Cyan = the active locked point; red = has StateTags (e.g. Block.Stunned); green = free.
		const bool bActive = (Point == ActivePoint);
		const bool bBlocked = !Point->StateTags.IsEmpty();
		const FColor Color = bActive ? FColor::Cyan : (bBlocked ? FColor::Red : FColor::Green);
		AddShape(FGameplayDebuggerShape::MakePoint(Point->GetComponentLocation(), bActive ? 12.0f : 8.0f, Color));

		const FString PointTagsStr = Point->PointTags.IsEmpty() ? TEXT("(no tags)") : Point->PointTags.ToStringSimple();
		const FString ActiveStr = bActive ? TEXT(" {cyan}[active]") : TEXT("");
		const FString StateStr = Point->StateTags.IsEmpty()
			? FString()
			: FString::Printf(TEXT(" {red}[%s]"), *Point->StateTags.ToStringSimple());

		AddTextLine(FString::Printf(TEXT("    %s{white}%s%s%s"),
			bActive ? TEXT("{cyan}* ") : (bBlocked ? TEXT("{red}* ") : TEXT("{green}* ")),
			*PointTagsStr,
			*ActiveStr,
			*StateStr));
	}
}

#endif
