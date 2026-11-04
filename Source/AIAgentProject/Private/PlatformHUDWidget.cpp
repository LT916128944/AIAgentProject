#include "PlatformHUDWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "FpsPawn.h"
#include "PlatformProgressComponent.h"
#include "TimerManager.h"

void UPlatformHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 通关/失败提示开场先收起；收齐或超时由 Refresh 打开
    if (txt_Clear)
    {
        txt_Clear->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (txt_Fail)
    {
        txt_Fail->SetVisibility(ESlateVisibility::Collapsed);
    }

    BindProgress();

    // 倒计时读数每帧在变。Widget 的 NativeTick 要蓝图侧也实现才会被驱动，
    // 这里用 0.05 秒的定时器，读数一样跟得上，且不依赖蓝图 Tick 开关。
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RefreshTimer, this, &UPlatformHUDWidget::Refresh, 0.05f, /*bLoop=*/true);
    }
    Refresh();
}

void UPlatformHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefreshTimer);
    }
    Super::NativeDestruct();
}

void UPlatformHUDWidget::BindProgress()
{
    UPlatformProgressComponent* Progress = GetProgress();
    if (!Progress || BoundProgress.Get() == Progress)
    {
        return;
    }
    Progress->OnCoinCollectedDyn.AddDynamic(this, &UPlatformHUDWidget::HandleCoinCollected);
    Progress->OnAllCollectedDyn.AddDynamic(this, &UPlatformHUDWidget::HandleAllCollected);
    Progress->OnTimeOutDyn.AddDynamic(this, &UPlatformHUDWidget::HandleTimeOut);
    BoundProgress = Progress;
}

UPlatformProgressComponent* UPlatformHUDWidget::GetProgress() const
{
    if (UPlatformProgressComponent* Cached = BoundProgress.Get())
    {
        return Cached;
    }
    return APlatformProgressActor::FindWorldProgress(GetWorld());
}

void UPlatformHUDWidget::Refresh()
{
    // 手榴弹读数独立于进度中枢：FPS 关没有 Progress，放在早退之前才会刷
    if (txt_Grenade)
    {
        if (const AFpsPawn* Pawn = Cast<AFpsPawn>(GetOwningPlayerPawn()))
        {
            txt_Grenade->SetText(FText::FromString(
                FString::Printf(TEXT("手榴弹 %d"), Pawn->GrenadeCount)));
        }
    }

    UPlatformProgressComponent* Progress = GetProgress();
    if (!Progress)
    {
        return;
    }
    if (txt_CoinCount)
    {
        txt_CoinCount->SetText(FText::FromString(
            FString::Printf(TEXT("金币 %d/%d"), Progress->CollectedCount, Progress->TotalCoins)));
    }
    if (txt_Timer)
    {
        txt_Timer->SetText(FText::FromString(FormatCountdown(Progress->RemainingSeconds)));
    }
    if (txt_Clear)
    {
        // 控件树里两个提示文本是默认 "Text Block"，文案由这里给，免得人还要再改一次图
        txt_Clear->SetText(FText::FromString(TEXT("通关")));
        txt_Clear->SetVisibility(
            Progress->bAllCollected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
    if (txt_Fail)
    {
        txt_Fail->SetText(FText::FromString(TEXT("时间到")));
        txt_Fail->SetVisibility(
            Progress->bTimeOut ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

FString UPlatformHUDWidget::FormatCountdown(float Seconds)
{
    const int32 Total = FMath::Max(0, FMath::CeilToInt(Seconds));
    return FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60);
}

void UPlatformHUDWidget::HandleCoinCollected(int32 CollectedCount)
{
    Refresh();
}

void UPlatformHUDWidget::HandleAllCollected()
{
    Refresh();
}

void UPlatformHUDWidget::HandleTimeOut()
{
    Refresh();
}
