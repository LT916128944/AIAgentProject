#include "TestEnemy.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TestPawn.h"

ATestEnemy::ATestEnemy()
{
    PrimaryActorTick.bCanEverTick = true;

    // 没有场景根时 SetActorLocation 不会改变 GetActorLocation，巡逻位移读出来一直是 0。
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);

    PatrolComponent = CreateDefaultSubobject<UTestEnemyPatrolComponent>(TEXT("Patrol"));
    PrintTestLog();
}

void ATestEnemy::PrintTestLog()
{
    UE_LOG(LogTemp, Log, TEXT("ATestEnemy PrintTestLog"));
}

void ATestEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 进入关卡后先把行为拉回巡逻，并启动巡逻组件
    TargetPawn = nullptr;
    AttackCooldownRemaining = 0.0f;
    SetState(ETestEnemyState::Patrol);

    if (PatrolComponent)
    {
        PatrolComponent->StartPatrol();
    }

    UE_LOG(LogTemp, Log, TEXT("ATestEnemy BeginPlay: state=Patrol"));
}

void ATestEnemy::InitPatrol(float InPatrolRadius, float InMoveSpeed)
{
    if (!PatrolComponent)
    {
        return;
    }

    PatrolComponent->PatrolRadius = InPatrolRadius;
    PatrolComponent->MoveSpeed = InMoveSpeed;
    PatrolComponent->StartPatrol();

    UE_LOG(LogTemp, Log, TEXT("ATestEnemy InitPatrol: radius=%.1f speed=%.1f"), InPatrolRadius, InMoveSpeed);
}

void ATestEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 攻击冷却计时
    if (AttackCooldownRemaining > 0.0f)
    {
        AttackCooldownRemaining = FMath::Max(0.0f, AttackCooldownRemaining - DeltaSeconds);
    }

    // 校验当前目标是否仍然有效，失效则回到巡逻
    if (TargetPawn && !IsValid(TargetPawn))
    {
        TargetPawn = nullptr;
        SetState(ETestEnemyState::Patrol);
    }

    switch (CurrentState)
    {
    case ETestEnemyState::Patrol:
        TickPatrol(DeltaSeconds);
        break;
    case ETestEnemyState::Chase:
        TickChase(DeltaSeconds, TargetPawn);
        break;
    case ETestEnemyState::Attack:
        TickAttack(DeltaSeconds, TargetPawn);
        break;
    default:
        break;
    }
}

ATestPawn* ATestEnemy::FindNearestPlayer() const
{
    ATestPawn* Nearest = nullptr;
    float NearestDistSq = TNumericLimits<float>::Max();

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    const FVector MyLocation = GetActorLocation();

    for (TActorIterator<ATestPawn> It(World); It; ++It)
    {
        ATestPawn* Candidate = *It;
        if (!IsValid(Candidate))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(MyLocation, Candidate->GetActorLocation());
        if (DistSq < NearestDistSq)
        {
            NearestDistSq = DistSq;
            Nearest = Candidate;
        }
    }

    return Nearest;
}

void ATestEnemy::TickPatrol(float DeltaSeconds)
{
    // 巡逻期间持续寻找玩家，进入探测范围就转入追击
    ATestPawn* Player = FindNearestPlayer();
    if (!Player)
    {
        return;
    }

    const float DistSq = FVector::DistSquared(GetActorLocation(), Player->GetActorLocation());
    if (DistSq <= FMath::Square(DetectionRange))
    {
        TargetPawn = Player;
        SetState(ETestEnemyState::Chase);
        TickChase(DeltaSeconds, Player);
    }
}

void ATestEnemy::TickChase(float DeltaSeconds, ATestPawn* Player)
{
    if (!IsValid(Player))
    {
        TargetPawn = nullptr;
        SetState(ETestEnemyState::Patrol);
        return;
    }

    const FVector MyLocation = GetActorLocation();
    const FVector TargetLocation = Player->GetActorLocation();
    const float DistSq = FVector::DistSquared(MyLocation, TargetLocation);

    // 目标超出丢失范围，放弃追击
    const float LoseRange = DetectionRange * LoseTargetRangeMultiplier;
    if (DistSq > FMath::Square(LoseRange))
    {
        TargetPawn = nullptr;
        SetState(ETestEnemyState::Patrol);
        return;
    }

    // 进入攻击距离，转入攻击
    if (DistSq <= FMath::Square(AttackRange))
    {
        SetState(ETestEnemyState::Attack);
        TickAttack(DeltaSeconds, Player);
        return;
    }

    // 朝向玩家并推进（停在 ChaseStopRange 之外）
    const FVector ToTarget = TargetLocation - MyLocation;
    const float Distance = ToTarget.Size();
    if (Distance > KINDA_SMALL_NUMBER)
    {
        const FVector Direction = ToTarget / Distance;
        SetActorRotation(Direction.Rotation());

        if (Distance > ChaseStopRange)
        {
            const FVector Delta = Direction * FMath::Min(ChaseSpeed * DeltaSeconds, Distance - ChaseStopRange);
            AddActorWorldOffset(Delta, /*bSweep=*/true);
        }
    }
}

void ATestEnemy::TickAttack(float DeltaSeconds, ATestPawn* Player)
{
    if (!IsValid(Player))
    {
        TargetPawn = nullptr;
        SetState(ETestEnemyState::Patrol);
        return;
    }

    const FVector MyLocation = GetActorLocation();
    const FVector TargetLocation = Player->GetActorLocation();
    const float Distance = FVector::Dist(MyLocation, TargetLocation);

    // 距离拉开则回到追击
    if (Distance > AttackRange)
    {
        SetState(ETestEnemyState::Chase);
        return;
    }

    // 面向玩家
    const FVector ToTarget = TargetLocation - MyLocation;
    if (!ToTarget.IsNearlyZero())
    {
        SetActorRotation(ToTarget.Rotation());
    }

    // 冷却结束则攻击
    if (AttackCooldownRemaining <= 0.0f)
    {
        ApplyAttack(Player);
        AttackCooldownRemaining = AttackCooldown;
    }
}

void ATestEnemy::SetState(ETestEnemyState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    CurrentState = NewState;
}

void ATestEnemy::ApplyAttack(ATestPawn* Player)
{
    if (!IsValid(Player))
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ATestEnemy attacks %s for %.1f damage"), *Player->GetName(), AttackDamage);

    UGameplayStatics::ApplyDamage(Player, AttackDamage, GetInstigatorController(), this, UDamageType::StaticClass());
}

float ATestEnemy::GetDistanceToTarget() const
{
    if (!IsValid(TargetPawn))
    {
        return -1.0f;
    }

    // 只算水平距离，避免高度差影响判断
    FVector Delta = TargetPawn->GetActorLocation() - GetActorLocation();
    Delta.Z = 0.0f;
    return Delta.Size();
}

bool ATestEnemy::HasTarget() const
{
    return IsValid(TargetPawn);
}
