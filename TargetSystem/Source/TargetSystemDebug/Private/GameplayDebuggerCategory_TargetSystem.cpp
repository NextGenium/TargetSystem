// Copyright (c) 2024 NextGenium

#include "GameplayDebuggerCategory_TargetSystem.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "TargetPointComponent.h"
#include "TargetLockComponent.h"
#include "TargetSystemInterface.h"
#include "TargetSystemDebugDraw.h"
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

#if ENABLE_DRAW_DEBUG
	// Reuse the cvar overlay's foreground (x-ray) draw so simply opening the debugger (apostrophe)
	// reproduces the through-mesh trace + point markers + labels with NO console command. The category's
	// own AddShape markers are SDPG_World (occluded by the mesh) — only their Description text reads
	// through. Level 3 = trace + anchor + points with name/point-tag + blocker-tag labels.
	TargetSystemDebugDraw::DrawOverlay(Component, 3);
#endif

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

	// Anchor the trace at the actual locked POINT we are attached to, not the actor origin.
	const UTargetPointComponent* ActivePoint = Component->GetLockedPoint();
	const FVector AnchorLocation = IsValid(ActivePoint) ? ActivePoint->GetComponentLocation() : Target->GetActorLocation();

	const float TargetDistance = FVector::Dist(PawnLocation, Target->GetActorLocation());
	AddTextLine(FString::Printf(TEXT("{white}Target: {yellow}%s  {white}dist {yellow}%.0f {white}/ lose {grey}%.0f"),
		*GetNameSafe(Target), TargetDistance, Component->GetLoseTargetDistance()));

	// 3D line player -> locked point + marker on the anchor.
	AddShape(FGameplayDebuggerShape::MakeSegment(PawnLocation, AnchorLocation, 2.0f, FColor::Yellow));
	AddShape(FGameplayDebuggerShape::MakePoint(AnchorLocation, 12.0f, FColor::Yellow));

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

		// Green = lockable point; red = has StateTags (e.g. Block.Stunned). The active point shares the
		// green but is drawn larger — the yellow trace marks which one we are anchored to.
		const bool bActive = (Point == ActivePoint);
		const bool bBlocked = !Point->StateTags.IsEmpty();
		const FColor Color = bBlocked ? FColor::Red : FColor::Green;

		// World marker + label right at the point. Gameplay-debugger shapes draw on the canvas
		// (screen-projected), so both the marker and its Description text read THROUGH the target
		// mesh — no cvar needed, this shows the moment the category is enabled. Label mirrors the
		// MotionWarping point visualizer: "Name [point tags]".
		const FString PointTagsStr = Point->PointTags.IsEmpty() ? TEXT("(no point tags)") : Point->PointTags.ToStringSimple();
		FString PointLabel = FString::Printf(TEXT("%s [%s]"), *Point->GetName(), *PointTagsStr);
		if (bActive)
		{
			PointLabel += TEXT(" [active]");
		}
		AddShape(FGameplayDebuggerShape::MakePoint(Point->GetComponentLocation(), bActive ? 16.0f : 12.0f, Color, PointLabel));

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
