// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemInterface.generated.h"

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
    virtual UTargetSystemDependencies* GetTargetSystemDependencies() {return nullptr; }
    virtual bool IsTargetable() const { return false; }

    virtual void StartTargetable() {}
    virtual void StopTargetable() {}
};
