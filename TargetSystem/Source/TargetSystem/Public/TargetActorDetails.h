// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetActorDetails.generated.h"

class UBTargetPoint;

USTRUCT(Blueprintable)
struct FTargetActorDetails
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCouldBeTarget = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString StartTargetPointName = "None";

    UPROPERTY()
    TArray<UBTargetPoint*> TargetPoints{};

    bool bIsTargetable = false;
};
