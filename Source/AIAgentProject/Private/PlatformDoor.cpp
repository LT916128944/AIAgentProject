#include "PlatformDoor.h"

#include "Components/SceneComponent.h"
#include "PlatformProgressComponent.h"

APlatformDoor::APlatformDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 移动读数靠 Movable 场景根（巡敌同款约定）
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);
}

void APlatformDoor::BeginPlay()
{
    Super::BeginPlay();

    // 关卡里自动绑；零个或多个时不绑，不猜。测试与蓝图可显式传（BindToProgress）
    UPlatformProgressComponent* Progress = APlatformProgressActor::FindWorldProgress(GetWorld());
    if (Progress)
    {
        BindToProgress(Progress);
    }
}

void APlatformDoor::BindToProgress(UPlatformProgressComponent* InProgress)
{
    if (!InProgress || bIsOpen)
    {
        return;
    }
    InProgress->OnAllCollectedDyn.AddDynamic(this, &APlatformDoor::HandleAllCollected);
}

void APlatformDoor::Open()
{
    if (bIsOpen)
    {
        return;
    }
    bIsOpen = true;
    AddActorWorldOffset(OpenOffset, /*bSweep=*/false);
    OnOpened();
}

void APlatformDoor::HandleAllCollected()
{
    Open();
}