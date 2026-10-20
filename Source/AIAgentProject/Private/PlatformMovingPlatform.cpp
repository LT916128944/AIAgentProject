#include "PlatformMovingPlatform.h"

#include "Components/SceneComponent.h"

AMovingPlatform::AMovingPlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    // 同款约定：Movable 场景根，位置读数才会动
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);
}

void AMovingPlatform::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();
    if (bStartActive)
    {
        StartMoving();
    }
}

void AMovingPlatform::StartMoving()
{
    bMoving = MoveDelta.SizeSquared() > KINDA_SMALL_NUMBER && MoveSpeed > 0.f;
}

void AMovingPlatform::StopMoving()
{
    bMoving = false;
}

void AMovingPlatform::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bMoving)
    {
        return;
    }
    const float Length = MoveDelta.Size();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }
    Alpha += Direction * (MoveSpeed * DeltaSeconds) / Length;
    if (Alpha >= 1.f)
    {
        Alpha = 1.f;
        Direction = -1;
    }
    else if (Alpha <= 0.f)
    {
        Alpha = 0.f;
        Direction = 1;
    }
    SetActorLocation(StartLocation + MoveDelta * Alpha);
}