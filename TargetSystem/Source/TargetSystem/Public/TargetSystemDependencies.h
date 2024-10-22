// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetActorDetails.h"
#include "UObject/Object.h"
#include "TargetSystemDependencies.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TARGETSYSTEM_API UTargetSystemDependencies final : public UActorComponent
{
    GENERATED_BODY()

public:
    FTargetActorDetails GetTargetActorDetails() const { return TargetActorDetails; }
    void SetIsTargetable(bool Value) {TargetActorDetails.bIsTargetable = Value; }

    void SetUp(TArray<UBTargetPoint*> TargetPoints);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Details")
    FTargetActorDetails TargetActorDetails;
};
