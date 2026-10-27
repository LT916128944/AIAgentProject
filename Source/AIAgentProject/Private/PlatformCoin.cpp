#include "PlatformCoin.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "PlatformProgressComponent.h"

ACoin::ACoin()
{
    PrimaryActorTick.bCanEverTick = true;

    // 根组件是碰撞球：SetActorLocation 跟着走，也是「离玩家多近算碰到」的可视半径
    USphereComponent* Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    Sphere->InitSphereRadius(PickupRadius);
    Sphere->SetMobility(EComponentMobility::Movable);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true);
    SetRootComponent(Sphere);
    PickupSphere = Sphere;

    // 引擎旋转组件：默认绕 Z 轴每秒 180 度。发光表现（MID Emissive）由人或蓝图半边接
    Rotating = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Rotating"));
    Rotating->RotationRate = FRotator(0.f, 180.f, 0.f);
}

void ACoin::BeginPlay()
{
    Super::BeginPlay();

    // 先找进度中枢自注册。零个或多个时不动：总数留给关卡装配者裁决
    UPlatformProgressComponent* Progress = APlatformProgressActor::FindWorldProgress(GetWorld());
    if (Progress)
    {
        Progress->RegisterPendingCoin();
    }
}

void ACoin::NotifyCollected(AActor* Collector)
{
    if (bCollected)
    {
        return;
    }
    bCollected = true;

    if (PickupSphere)
    {
        PickupSphere->SetGenerateOverlapEvents(false);
        PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    SetActorHiddenInGame(true);

    UPlatformProgressComponent* Progress = APlatformProgressActor::FindWorldProgress(GetWorld());
    if (Progress)
    {
        Progress->NotifyCoinCollected();
    }

    OnCollected();
}

AActor* ACoin::FindPlayerPawn() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* Player = It->Get())
        {
            if (APawn* Pawn = Player->GetPawn())
            {
                return Pawn;
            }
        }
    }
    return nullptr;
}

void ACoin::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bCollected || !PickupSphere)
    {
        return;
    }
    AActor* Player = FindPlayerPawn();
    if (!Player)
    {
        return;
    }
    // 用球心到 Pawn 根的距离。设计图里金币就在平台上方 80uu，站上去就该收到
    const float Reach = PickupRadius;
    if (FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) <= Reach * Reach)
    {
        NotifyCollected(Player);
    }
}