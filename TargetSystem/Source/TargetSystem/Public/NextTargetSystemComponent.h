// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "TargetSystemComponent.h"
#include "NextTargetSystemComponent.generated.h"

struct FTargetingRequestHandle;
class UTargetingPreset;
/**
 * 
 */
UCLASS()
class TARGETSYSTEM_API UNextTargetSystemComponent : public UTargetSystemComponent
{
	GENERATED_BODY()

public:
	virtual void TryStartTargetLock() override;
	virtual void SwitchTarget(FVector2D AxisValue) override;
	virtual void AutoSwitchTarget() override;
	virtual void StopObservingTarget(const bool bIgnoreAutoSwitch = false, const bool bTargetIsDead = false) override;

	UFUNCTION()
	void OnTargetingCompleted(FTargetingRequestHandle Handle);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem",
		meta = (InlineEditConditionToggle))
	bool bUseTargetSubsystem = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem",
		meta=(EditCondition="bUseTargetSubsystem"))
	UTargetingPreset* TargetingPreset = nullptr;
	
private:
	void ClearTargetingHandles();
	
	UPROPERTY()
	TArray<FTargetingRequestHandle> TargetingHandles;
};
