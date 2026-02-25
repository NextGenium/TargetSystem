// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemInterface.generated.h"

class UNextTargetSystemComponent;
class UTargetSystemComponent;
class UTargetSystemDependencies;

UINTERFACE(Blueprintable)
class UTargetSystemInterface : public UInterface
{
	GENERATED_BODY()
};

class TARGETSYSTEM_API ITargetSystemInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "NextTargetSystem")
	UNextTargetSystemComponent* GetTargetSystemComponent() const;
	virtual UNextTargetSystemComponent* GetTargetSystemComponent_Implementation() const;
	
    virtual UTargetSystemDependencies* GetTargetSystemDependencies() {return nullptr; }
    virtual bool IsTargetable() const { return false; }

    virtual void StartTargetable() {}
    virtual void StopTargetable() {}
};
