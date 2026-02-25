// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemOwnerInterface.generated.h"

class UNextTargetSystemComponent;
class USpringArmComponent;

UINTERFACE(Blueprintable)
class UTargetSystemOwnerInterface : public UInterface
{
    GENERATED_BODY()
};

// TODO: удалить, я не уверен что нужно 2 отдельных интерфейса иметь, 
// скорее по GetIsOwner проверять или чёт такое
class TARGETSYSTEM_API ITargetSystemOwnerInterface
{
    GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NextTargetSystem")
	UNextTargetSystemComponent* GetTargetSystemComponent() const;
	virtual UNextTargetSystemComponent* GetTargetSystemComponent_Implementation() const;

	virtual FVector GetCameraLocation() const { return {}; }

    virtual void ChangeCameraLocation(const FVector& Location) {}
};
