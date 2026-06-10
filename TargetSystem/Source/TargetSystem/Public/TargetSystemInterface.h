// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemInterface.generated.h"

class UTargetPointComponent;
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

    // DEPRECATED: lock-state hooks on the target. Removed when the component
    // refactor lands (Step 8) and the lifecycle moves to OnTargetLockBegin/End.
    virtual void StartTargetable() {}
    virtual void StopTargetable() {}
};
