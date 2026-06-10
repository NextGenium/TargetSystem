// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "OverrideCameraDistanceVolume.generated.h"

class ITargetSystemOwnerInterface;
class UTargetSystemComponent;
class ITargetSystemInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnTriggerActivated,
    bool,IsOn
);

UCLASS(PrioritizeCategories = "Volume Settings")
class TARGETSYSTEM_API AOverrideCameraDistanceVolume : public AActor
{
    GENERATED_BODY()

public:
    AOverrideCameraDistanceVolume();

    UFUNCTION(BlueprintCallable)
    bool IsActivateVolume() const { return bIsActivate; }

    UFUNCTION(BlueprintCallable)
    void ActivateVolume();

    UFUNCTION(BlueprintCallable)
    void DeactivateVolume();

    UPROPERTY(BlueprintAssignable) FOnTriggerActivated OnTriggerActivated;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Settings")
    UCurveFloat* TimelineCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Settings")
    FVector NeedSpringArmSocketOffset = FVector::Zero();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Volume Settings")
    bool bInitiallyActivate = true;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Volume Settings")
    TArray<AActor*> EnemiesInVolume;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = " Billboard Settings")
    float BillboardScaleCoefficient = 0.1f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UBillboardComponent* BillboardComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UPrimitiveComponent* InteractionVolume;

    UFUNCTION()
    virtual void OnInteractionVolumeOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

    UFUNCTION()
    virtual void OnInteractionVolumeOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
    UPROPERTY()
    TArray<TScriptInterface<ITargetSystemInterface>> TargetsInVolume {};

    UPROPERTY()
    TScriptInterface<ITargetSystemOwnerInterface> PlayerInterface;

    UPROPERTY()
    UTargetSystemComponent* TargetSystemComponent = nullptr;

    FVector CurrentSpringArmSocketOffset = FVector::Zero();
    FTimeline CameraDistanceTimeline;
    bool bIsActivate = false;

    void StartLogic();
    void StopLogic();
    void UpdateSocketOffset(float Alpha);

    UFUNCTION()
    void ChangeTargetsInVolume(TScriptInterface<ITargetSystemInterface> DeletedInterface);
};
