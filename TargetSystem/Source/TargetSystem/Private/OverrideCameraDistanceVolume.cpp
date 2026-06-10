// Fill out your copyright notice in the Description page of Project Settings.

#include "OverrideCameraDistanceVolume.h"

#include "TargetSystemComponent.h"
#include "TargetSystemDependencies.h"
#include "TargetSystemInterface.h"
#include "TargetSystemOwnerInterface.h"
#include "Components/BoxComponent.h"
#include "Components/TimelineComponent.h"

AOverrideCameraDistanceVolume::AOverrideCameraDistanceVolume()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>("Root");

    BillboardComponent = CreateDefaultSubobject<UBillboardComponent>("BillboardComponent");
    BillboardComponent->SetupAttachment(RootComponent);

    InteractionVolume = CreateDefaultSubobject<UBoxComponent>("InteractionVolume");
    InteractionVolume->SetupAttachment(RootComponent);
    InteractionVolume->SetGenerateOverlapEvents(true);
}

void AOverrideCameraDistanceVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const float NewBillboardComponentScaleX = InteractionVolume->GetComponentScale().Length() * BillboardScaleCoefficient;
    FVector BillboardComponentScale = FVector::ZeroVector;
    BillboardComponentScale.X = FMath::Clamp(NewBillboardComponentScaleX, 1.f, NewBillboardComponentScaleX * 2);
    BillboardComponent->SetWorldScale3D(BillboardComponentScale);
}

void AOverrideCameraDistanceVolume::BeginPlay()
{
    Super::BeginPlay();

    if (!EnemiesInVolume.IsEmpty())
    {
        for (AActor* Actor : EnemiesInVolume)
        {
            auto Interface = StaticCast<TScriptInterface<ITargetSystemInterface>>(Actor);
            if (!Interface) continue;
            if (!Interface->GetTargetSystemDependencies()->GetTargetActorDetails().bCouldBeTarget) continue;

            TargetsInVolume.Add(Interface);
        }
        EnemiesInVolume.Empty();
    }

    if (bInitiallyActivate)
    {
        ActivateVolume();
    }

    if(IsValid(TimelineCurve))
    {
        FOnTimelineFloatStatic MovingTimelineUpdate;
        MovingTimelineUpdate.BindUObject(this, &AOverrideCameraDistanceVolume::UpdateSocketOffset);
        CameraDistanceTimeline.AddInterpFloat(TimelineCurve, MovingTimelineUpdate);
    }
}

void AOverrideCameraDistanceVolume::Tick(float DeltaSeconds)
{
    CameraDistanceTimeline.TickTimeline(DeltaSeconds);
}

void AOverrideCameraDistanceVolume::ActivateVolume()
{
    if (TargetsInVolume.IsEmpty() || bIsActivate) return;
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AOverrideCameraDistanceVolume::OnInteractionVolumeOverlapBegin);
        InteractionVolume->OnComponentEndOverlap.AddDynamic(this, &AOverrideCameraDistanceVolume::OnInteractionVolumeOverlapEnd);
    }

    TArray<AActor*> OverlappingActors;
    InteractionVolume->UpdateOverlaps();
    InteractionVolume->GetOverlappingActors(OverlappingActors);
    for (AActor* OverlappingActor : OverlappingActors)
    {
        if (!OverlappingActor->Implements<UTargetSystemOwnerInterface>()) continue;

        const auto OwnerInterface = StaticCast<TScriptInterface<ITargetSystemOwnerInterface>>(OverlappingActor);
        if (!OwnerInterface) return;

        PlayerInterface = OwnerInterface;
        StartLogic();

        if (OnTriggerActivated.IsBound())
        {
            OnTriggerActivated.Broadcast(true);
        }
        return;
    }
}

void AOverrideCameraDistanceVolume::DeactivateVolume()
{
    if (!bIsActivate) return;

    bIsActivate = false;
    CameraDistanceTimeline.Reverse();
    PlayerInterface->GetTargetSystemComponent()->OnTargetIsDead.Remove( this, "ChangeTargetsInVolume");

    if (IsValid(InteractionVolume))
    {
        InteractionVolume->OnComponentBeginOverlap.Remove(this, "OnInteractionVolumeOverlapBegin");
        InteractionVolume->OnComponentEndOverlap.Remove(this, "OnInteractionVolumeOverlapEnd");
    }
}

void AOverrideCameraDistanceVolume::OnInteractionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (TargetsInVolume.IsEmpty()) return;

    const auto OwnerInterface = StaticCast<TScriptInterface<ITargetSystemOwnerInterface>>(OtherActor);
    if (!OwnerInterface) return;

    PlayerInterface = OwnerInterface;
    StartLogic();

    if (OnTriggerActivated.IsBound())
    {
        OnTriggerActivated.Broadcast(true);
    }
}

void AOverrideCameraDistanceVolume::OnInteractionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!bIsActivate) return;

    const auto OwnerInterface = StaticCast<TScriptInterface<ITargetSystemOwnerInterface>>(OtherActor);
    if (!OwnerInterface) return;

    bIsActivate = false;
    CameraDistanceTimeline.Reverse();
    PlayerInterface->GetTargetSystemComponent()->OnTargetIsDead.Remove( this, "ChangeTargetsInVolume");
}

void AOverrideCameraDistanceVolume:: StartLogic()
{
    bIsActivate = true;

    if (IsValid(PlayerInterface->GetTargetSystemComponent()))
    {
        PlayerInterface->GetTargetSystemComponent()->OnTargetIsDead.AddDynamic( this, &AOverrideCameraDistanceVolume::ChangeTargetsInVolume);
    }

    CurrentSpringArmSocketOffset = PlayerInterface->GetCameraLocation();
    CameraDistanceTimeline.PlayFromStart();
}

void AOverrideCameraDistanceVolume::StopLogic()
{
    if (PlayerInterface)
    {
        PlayerInterface = nullptr;
    }

    if (OnTriggerActivated.IsBound())
    {
        OnTriggerActivated.Broadcast(false);
    }
}

void AOverrideCameraDistanceVolume::ChangeTargetsInVolume(TScriptInterface<ITargetSystemInterface> DeletedInterface)
{
    if (!TargetsInVolume.Contains(DeletedInterface)) return;

    TargetsInVolume.Remove(DeletedInterface);
    if (TargetsInVolume.IsEmpty())
    {
        bIsActivate = false;
        CameraDistanceTimeline.Reverse();
    }
}

void AOverrideCameraDistanceVolume::UpdateSocketOffset(float Alpha)
{
    if (!PlayerInterface) return;
    PlayerInterface->ChangeCameraLocation(FMath::Lerp(CurrentSpringArmSocketOffset, NeedSpringArmSocketOffset, Alpha));
    if (Alpha <= 0.f && !bIsActivate)
    {
        StopLogic();
    }
}
