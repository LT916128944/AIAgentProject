#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlatformHUDWidget.generated.h"

class UPlatformProgressComponent;
class UTextBlock;

/**
 * 平台跳跃 HUD 的 C++ 半边。控件树仍由 WBP_HUD 摆（UMG 连线走人工清单），
 * 这里只按名字找四个合同控件，把进度组件上的计数、倒计时、通关/失败读成文本。
 * 读法只有一个入口 Refresh()：0.05 秒一次的定时器驱动，外加三个委托立刻刷一次。
 * HUD 计数 倒计时 通关 失败
 */
UCLASS()
class AIAGENTPROJECT_API UPlatformHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 找关卡里唯一的进度中枢并绑三个委托。零个或多个时不绑，不猜。
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void BindProgress();

    UFUNCTION(BlueprintPure, Category = "Platform")
    UPlatformProgressComponent* GetProgress() const;

    // 把进度组件上的数读进四个控件。没有进度组件时不动控件。
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void Refresh();

    // 剩余秒数 → "1:30"。分秒补零，负数按 0
    UFUNCTION(BlueprintPure, Category = "Platform")
    static FString FormatCountdown(float Seconds);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "Platform", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> txt_CoinCount;

    UPROPERTY(BlueprintReadOnly, Category = "Platform", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> txt_Timer;

    UPROPERTY(BlueprintReadOnly, Category = "Platform", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> txt_Clear;

    UPROPERTY(BlueprintReadOnly, Category = "Platform", meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> txt_Fail;

private:
    UFUNCTION()
    void HandleCoinCollected(int32 CollectedCount);

    UFUNCTION()
    void HandleAllCollected();

    UFUNCTION()
    void HandleTimeOut();

    TWeakObjectPtr<UPlatformProgressComponent> BoundProgress;
    FTimerHandle RefreshTimer;
};
