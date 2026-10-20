#include "PlatformCoin.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "PlatformProgressComponent.h"

ACoin::ACoin()
{
    PrimaryActorTick.bCanEverTick = true;

    // 根组件是碰撞球：SetActorLocation 跟着走，重叠事件也从这里发
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

    if (PickupSphere)
    {
        PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ACoin::HandleSphereOverlap);
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

void ACoin::HandleSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                bool bFromSweep, const FHitResult& SweepResult)
{
    // 只认 Pawn（玩家或 AI）。其他重叠不算拾取
    if (!OtherActor || !OtherActor->IsA<APawn>())
    {
        return;
    }
    NotifyCollected(OtherActor);
}