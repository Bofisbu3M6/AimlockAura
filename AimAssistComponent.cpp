#include "AimAssistComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UAimAssistComponent::UAimAssistComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UAimAssistComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAimAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Func)
{
    Super::TickComponent(DeltaTime, TickType, Func);

    if (!bAimActive)
    {
        if (CurrentTarget.IsValid())
            NotifyTargetChange(nullptr);
        return;
    }

    AActor* Best = FindBestTarget();
    NotifyTargetChange(Best);
    CurrentTarget = Best;

    if (CurrentTarget.IsValid())
    {
        FVector HeadLoc = GetHeadLocation(CurrentTarget.Get());
        SnapToLocation(DeltaTime, HeadLoc);

#if WITH_EDITOR
        DrawDebugSphere(GetWorld(), HeadLoc, 8.f, 8, FColor::Red, false, -1.f, 0, 1.f);
#endif
    }
}

void UAimAssistComponent::SetAimActive(bool bActive)
{
    bAimActive = bActive;

    if (!bActive)
    {
        NotifyTargetChange(nullptr);
        CurrentTarget = nullptr;
    }
}

FVector UAimAssistComponent::GetLockedHeadLocation() const
{
    if (!CurrentTarget.IsValid()) return FVector::ZeroVector;
    return GetHeadLocation(CurrentTarget.Get());
}

AActor* UAimAssistComponent::FindBestTarget() const
{
    APlayerController* PC = GetPC();
    if (!PC) return nullptr;

    FVector CamLoc;
    FRotator CamRot;
    GetCameraView(CamLoc, CamRot);
    FVector CamFwd = CamRot.Vector();

    TArray<AActor*> Enemies;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), EnemyTag, Enemies);

    AActor* BestActor = nullptr;
    float BestAngle = DetectionAngle;

    for (AActor* Enemy : Enemies)
    {
        if (!Enemy || Enemy == GetOwner()) continue;
        if (Enemy->ActorHasTag("Dead")) continue;

        FVector TargetLoc = GetHeadLocation(Enemy);
        FVector ToTarget = TargetLoc - CamLoc;
        float Dist = ToTarget.Size();

        if (Dist > DetectionRange) continue;

        float Angle = FMath::RadiansToDegrees(
            FMath::Acos(
                FMath::Clamp(FVector::DotProduct(CamFwd, ToTarget.GetSafeNormal()), -1.f, 1.f)
            ));

        if (Angle >= BestAngle) continue;

        if (!HasLineOfSight(CamLoc, TargetLoc)) continue;

        BestAngle = Angle;
        BestActor = Enemy;
    }

    return BestActor;
}

FVector UAimAssistComponent::GetHeadLocation(const AActor* Enemy) const
{
    if (!Enemy) return FVector::ZeroVector;

    const ACharacter* EnemyChar = Cast<ACharacter>(Enemy);
    if (EnemyChar)
    {
        USkeletalMeshComponent* Mesh = EnemyChar->GetMesh();
        if (Mesh)
        {
            for (const FName& SocketName : HeadSocketNames)
            {
                if (Mesh->DoesSocketExist(SocketName))
                    return Mesh->GetSocketLocation(SocketName);
            }
        }
    }

    FVector Origin, Extent;
    Enemy->GetActorBounds(false, Origin, Extent);
    return FVector(Origin.X, Origin.Y, Origin.Z + Extent.Z * 0.85f);
}

bool UAimAssistComponent::HasLineOfSight(const FVector& From, const FVector& To) const
{
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());
    if (CurrentTarget.IsValid())
        Params.AddIgnoredActor(CurrentTarget.Get());

    FHitResult Hit;
    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        From,
        To,
        FQuat::Identity,
        LOSChannel,
        FCollisionShape::MakeSphere(LOSTraceRadius),
        Params
    );

    return !bHit;
}

void UAimAssistComponent::SnapToLocation(float DeltaTime, const FVector& TargetLoc)
{
    APlayerController* PC = GetPC();
    if (!PC) return;

    FVector CamLoc;
    FRotator CamRot;
    GetCameraView(CamLoc, CamRot);

    FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(CamLoc, TargetLoc);

    FRotator NewRot;
    if (bInstantSnap)
        NewRot = LookAt;
    else
        NewRot = FMath::RInterpTo(CamRot, LookAt, DeltaTime, SnapInterpSpeed);

    NewRot.Roll = CamRot.Roll;
    PC->SetControlRotation(NewRot);
}

void UAimAssistComponent::GetCameraView(FVector& OutLoc, FRotator& OutRot) const
{
    APlayerController* PC = GetPC();
    if (PC)
    {
        PC->GetPlayerViewPoint(OutLoc, OutRot);
        return;
    }

    OutLoc = GetOwner()->GetActorLocation();
    OutRot = GetOwner()->GetActorRotation();
}

APlayerController* UAimAssistComponent::GetPC() const
{
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return nullptr;
    return Cast<APlayerController>(OwnerPawn->GetController());
}

void UAimAssistComponent::NotifyTargetChange(AActor* NewTarget)
{
    if (NewTarget == PreviousTarget.Get()) return;

    if (NewTarget != nullptr)
        OnTargetLocked.Broadcast(NewTarget);
    else if (PreviousTarget.IsValid())
        OnTargetLost.Broadcast();

    PreviousTarget = NewTarget;
}
