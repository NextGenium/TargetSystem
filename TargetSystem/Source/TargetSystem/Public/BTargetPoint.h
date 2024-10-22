// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "BTargetPoint.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), BlueprintType)
class TARGETSYSTEM_API UBTargetPoint final: public USceneComponent
{
	GENERATED_BODY()

public:
    int32 GetIndex() const { return Index; }
    UCurveFloat* GetPitchOffsetCurve() const { return PitchOffsetCurve; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "WARNING: Index must be different from the indexes of other TargetPoints. The order of switching between TargetPoints corresponds to the order of indexes."))
    int32 Index = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Offset using Curve")
    UCurveFloat* PitchOffsetCurve = nullptr;
};
