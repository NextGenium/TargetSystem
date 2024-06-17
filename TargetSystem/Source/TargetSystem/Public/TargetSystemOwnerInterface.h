// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemOwnerInterface.generated.h"

class USpringArmComponent;
class UTargetSystemComponent;

UINTERFACE(Blueprintable)
class UTargetSystemOwnerInterface : public UInterface
{
    GENERATED_BODY()
};

class TARGETSYSTEM_API ITargetSystemOwnerInterface
{
    GENERATED_BODY()

public:
    virtual UTargetSystemComponent* GetTargetSystemComponent() const { return nullptr; }
    virtual FVector GetCameraLocation() const { return {}; }

    virtual void ChangeCameraLocation(const FVector& Location) {}
};
