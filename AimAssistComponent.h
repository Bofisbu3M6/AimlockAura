#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimAssistComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetLocked, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTargetLost);

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent),
       DisplayName="Snap-to-Target Aim Assist")
class YOURGAME_API UAimAssistComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAimAssistComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection",
              meta=(ClampMin="1.0", ClampMax="60.0"))
    float DetectionAngle = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection")
    float DetectionRange = 5000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection")
    FName EnemyTag = "Enemy";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection")
    TArray<FName> HeadSocketNames = { "head", "Head", "Bip001_Head", "mixamorig:Head" };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection")
    float LOSTraceRadius = 20.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Detection")
    TEnumAsByte<ECollisionChannel> LOSChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Snap",
              meta=(ClampMin="0.5", ClampMax="50.0"))
    float SnapInterpSpeed = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim Assist|Snap")
    bool bInstantSnap = false;

    UPROPERTY(BlueprintAssignable, Category="Aim Assist|Events")
    FOnTargetLocked OnTargetLocked;

    UPROPERTY(BlueprintAssignable, Category="Aim Assist|Events")
    FOnTargetLost OnTargetLost;

    UFUNCTION(BlueprintCallable, Category="Aim Assist")
    void SetAimActive(bool bActive);

    UFUNCTION(BlueprintPure, Category="Aim Assist")
    bool IsLocked() const { return bAimActive && CurrentTarget.IsValid(); }

    UFUNCTION(BlueprintPure, Category="Aim Assist")
    AActor* GetLockedTarget() const { return CurrentTarget.Get(); }

    UFUNCTION(BlueprintPure, Category="Aim Assist")
    FVector GetLockedHeadLocation() const;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* Func) override;

private:
    bool bAimActive = false;
    TWeakObjectPtr<AActor> CurrentTarget;
    TWeakObjectPtr<AActor> PreviousTarget;

    APlayerController* GetPC() const;
    void GetCameraView(FVector& OutLoc, FRotator& OutRot) const;
    AActor* FindBestTarget() const;
    FVector GetHeadLocation(const AActor* Enemy) const;
    bool HasLineOfSight(const FVector& From, const FVector& To) const;
    void SnapToLocation(float DeltaTime, const FVector& TargetLoc);
    void NotifyTargetChange(AActor* NewTarget);
};
