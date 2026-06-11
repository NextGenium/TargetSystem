// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemInterface.generated.h"

class UTargetPointComponent;
class UTargetLockComponent;
struct FTargetPointQuery;

UINTERFACE(Blueprintable)
class UTargetSystemInterface : public UInterface
{
	GENERATED_BODY()
};

// Unified targetable contract implemented by enemy actors. Merges the former
// ITargetSystemTargetableInterface (IsTargetable) into a single interface and
// adds the tag-driven point queries + lock lifecycle callbacks (R3).
class TARGETSYSTEM_API ITargetSystemInterface
{
	GENERATED_BODY()

public:
    virtual bool IsTargetable() const { return false; }

    // Lock-on points on this target. Empty => caller falls back to GetActorLocation().
    virtual TArray<UTargetPointComponent*> GetTargetPoints() const { return {}; }

    // Points satisfying the query (target-side + source-side filtering, see R2).
    virtual TArray<UTargetPointComponent*> QueryTargetPoints(const FTargetPointQuery& Query) const { return {}; }

    // Lifecycle hooks for project UI / VFX / AI-reaction.
    virtual void OnTargetLockBegin(AActor* LockOwner) {}
    virtual void OnTargetLockEnd(AActor* LockOwner) {}

    // Owner-side (player) hooks, merged from the former ITargetSystemOwnerInterface (R3/L5).
    // The owner is the actor that exposes a TargetLockComponent — enemies return null.
    // Project actors override GetTargetSystemComponent_Implementation and call via
    // Execute_GetTargetSystemComponent.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Target System")
    UTargetLockComponent* GetTargetSystemComponent() const;
    virtual UTargetLockComponent* GetTargetSystemComponent_Implementation() const { return nullptr; }

    virtual FVector GetCameraLocation() const { return {}; }
    virtual void ChangeCameraLocation(const FVector& Location) {}
};
