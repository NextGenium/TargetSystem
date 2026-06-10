// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemOwnerInterface.generated.h"

class USpringArmComponent;
class UTargetLockComponent;

UINTERFACE(Blueprintable)
class UTargetSystemOwnerInterface : public UInterface
{
    GENERATED_BODY()
};

class TARGETSYSTEM_API ITargetSystemOwnerInterface
{
    GENERATED_BODY()

public:
    // BlueprintNativeEvent: project actors override GetTargetSystemComponent_Implementation
    // and call via Execute_GetTargetSystemComponent. Default returns nullptr.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Target System")
    UTargetLockComponent* GetTargetSystemComponent() const;
    virtual UTargetLockComponent* GetTargetSystemComponent_Implementation() const { return nullptr; }

    virtual FVector GetCameraLocation() const { return {}; }

    virtual void ChangeCameraLocation(const FVector& Location) {}
};
