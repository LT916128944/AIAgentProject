#include "PlatformProgressComponent.h"

#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

UPlatformProgressComponent::UPlatformProgressComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UPlatformProgressComponent::BeginPlay()
{
    Super::BeginPlay();
    StartCountdown();
}

void UPlatformProgressComponent::RegisterPendingCoin()
{
    ++TotalCoins;
}

void UPlatformProgressComponent::NotifyCoinCollected()
{
    ++CollectedCount;
    OnCoinCollected();
    OnCoinCollectedDyn.Broadcast(CollectedCount);
    if (TotalCoins > 0 && CollectedCount >= TotalCoins)
    {
        BroadcastAllCollected();
    }
}

void UPlatformProgressComponent::StartCountdown()
{
    RemainingSeconds = FMath::Max(0.f, CountdownSeconds);
    bCountdownRunning = RemainingSeconds > 0.f;
}

void UPlatformProgressComponent::ResetProgress(int32 InTotalCoins, float InCountdownSeconds)
{
    TotalCoins = FMath::Max(0, InTotalCoins);
    CountdownSeconds = FMath::Max(0.f, InCountdownSeconds);
    CollectedCount = 0;
    bAllCollected = false;
    bTimeOut = false;
    StartCountdown();
}

void UPlatformProgressComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bCountdownRunning || bAllCollected)
    {
        return;
    }
    RemainingSeconds = FMath::Max(0.f, RemainingSeconds - DeltaTime);
    if (RemainingSeconds <= 0.f)
    {
        bCountdownRunning = false;
        bTimeOut = true;
        OnTimeOut();
        OnTimeOutDyn.Broadcast();
    }
}

void UPlatformProgressComponent::BroadcastAllCollected()
{
    if (bAllCollected)
    {
        return;
    }
    bAllCollected = true;
    bCountdownRunning = false;
    OnAllCollected();
    OnAllCollectedDyn.Broadcast();
}

APlatformProgressActor::APlatformProgressActor()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);

    Progress = CreateDefaultSubobject<UPlatformProgressComponent>(TEXT("Progress"));
}

UPlatformProgressComponent* APlatformProgressActor::FindWorldProgress(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }
    UPlatformProgressComponent* Found = nullptr;
    int32 Count = 0;
    for (TActorIterator<APlatformProgressActor> It(World); It; ++It)
    {
        APlatformProgressActor* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->Progress)
        {
            continue;
        }
        ++Count;
        Found = Candidate->Progress;
    }
    // 零个或多个都不猜：关卡里只放一个进度中枢
    return Count == 1 ? Found : nullptr;
}