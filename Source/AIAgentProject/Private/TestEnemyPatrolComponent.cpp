#include "TestEnemyPatrolComponent.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UTestEnemyPatrolComponent::UTestEnemyPatrolComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PatrolRadius = 800.f;
    MoveSpeed = 300.f;
    bEnableChase = true;
    DetectRadius = 1200.f;
    AttackRange = 150.f;
    AttackInterval = 1.5f;
    PatrolAngle = 0.f;
    LastAttackTime = -1000.f;
    bChasing = false;
}

void UTestEnemyPatrolComponent::BeginPlay()
{
    Super::BeginPlay();
    StartPatrol();
}

void UTestEnemyPatrolComponent::StartPatrol()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    HomeLocation = Owner->GetActorLocation();
    bChasing = false;
    PatrolAngle = 0.f;
    PickPatrolTarget();
    OnPatrol();
}

void UTestEnemyPatrolComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    const FVector Here = Owner->GetActorLocation();
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (bEnableChase && Player)
    {
        const float Distance = FVector::Dist(Here, Player->GetActorLocation());
        if (Distance <= AttackRange)
        {
            UWorld* World = GetWorld();
            const float Now = World ? World->GetTimeSeconds() : 0.f;
            if (Now - LastAttackTime >= AttackInterval)
            {
                LastAttackTime = Now;
                OnMeleeAttack();
            }
            return;
        }
        if (Distance <= DetectRadius)
        {
            if (!bChasing)
            {
                bChasing = true;
                OnChase();
            }
            MoveToward(Player->GetActorLocation(), DeltaTime);
            return;
        }
    }

    if (bChasing)
    {
        bChasing = false;
        OnPatrol();
        PickPatrolTarget();
    }
    if (FVector::Dist(Here, PatrolTarget) <= 50.f)
    {
        PickPatrolTarget();
    }
    MoveToward(PatrolTarget, DeltaTime);
}

void UTestEnemyPatrolComponent::PickPatrolTarget()
{
    PatrolAngle += 90.f;
    const float Radians = FMath::DegreesToRadians(PatrolAngle);
    PatrolTarget = HomeLocation + FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f) * PatrolRadius;
}

void UTestEnemyPatrolComponent::MoveToward(const FVector& Target, float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner || MoveSpeed <= 0.f || DeltaTime <= 0.f)
    {
        return;
    }
    const FVector Here = Owner->GetActorLocation();
    const FVector Offset = Target - Here;
    const float Distance = Offset.Size();
    if (Distance <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    const float Step = FMath::Min(Distance, MoveSpeed * DeltaTime);
    Owner->SetActorLocation(Here + Offset.GetSafeNormal() * Step);
}
