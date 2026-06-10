// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TargetSystemDependencies.h"

#include "BTargetPoint.h"
#include "TargetSystemLog.h"

void UTargetSystemDependencies::SetUp(
    TArray<UBTargetPoint*> _TargetPoints
)
{
    if (!_TargetPoints.IsEmpty())
    {
        bool IsValidTargetPointName = false;
        for (UBTargetPoint* TargetPoint : _TargetPoints)
        {
            if (!IsValid(TargetPoint)) continue;

            if (!IsValidTargetPointName && TargetPoint->GetName() == TargetActorDetails.StartTargetPointName) IsValidTargetPointName = true;
            TargetActorDetails.TargetPoints.Add(TargetPoint);
        }

        if (TargetActorDetails.TargetPoints.IsEmpty())
        {
            UE_LOG(LogTargetSystem, Error, TEXT("No Target Points (%s)"), *GetOwner()->GetName());
            return;
        }

        TargetActorDetails.TargetPoints.Sort([this](const UBTargetPoint& A, const UBTargetPoint& B)
            {
                if (A.GetIndex() == B.GetIndex())
                {
                    UE_LOG(LogTargetSystem, Warning, TEXT("Identical Indexes in %s (%s and %s) = %i"), *this->GetName(), *A.GetName(), *B.GetName(), B.GetIndex());
                }
                return A.GetIndex() < B.GetIndex();
            }
        );

        if (!IsValidTargetPointName)
        {
            UE_LOG(LogTargetSystem, Warning, TEXT("StartTargetPointName not specified (%s)"), *this->GetName());
            TargetActorDetails.StartTargetPointName = TargetActorDetails.TargetPoints[0]->GetName();
        }
    }
}
