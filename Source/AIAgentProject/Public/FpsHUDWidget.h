#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FpsHUDWidget.generated.h"

class AFpsPawn;
class ASimpleVehiclePawn;
class UTextBlock;

/**
 * FPS HUD 的 C++ 半边（20261029-002746）。
 *
 * 控件树由 /Game/UI/WBP_FpsHUD 摆（UMG 连线走工具/人工），这里只按名字找六个合同控件，
 * 把 AFpsPawn 的弹药/手雷、载具的名字/时速/上下车提示、满血占位读成文本。
 *
 * 读法只有一个入口 Refresh()：0.05 秒一次的定时器驱动（UUserWidget::NativeTick 要蓝图侧
 * 实现 bHasScriptImplementedTick 才会被调，不依赖它）。挂视口由 AFpsPawn::EnsureHud 负责。
 *
 * 合同控件名（WBP 里必须同名，否则那一条显示为空）：
 *   txt_Health        左下角血量，能读到玩家血量时显示 Health01*MaxHealthPlaceholder，
 *                     读不到时回落到占位满血 MaxHealthPlaceholder（暂不接扣血）
 *   txt_Ammo          右下角子弹数量
 *   txt_Grenade       右下角手雷数量
 *   txt_VehicleName   右中部载具名字
 *   txt_EnterPrompt   右中部「按 E 上车/下车」
 *   txt_VehicleSpeed  右中部当前时速
 */
UCLASS()
class AIAGENTPROJECT_API UFpsHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 血量显示上限。数据口径（UFpsHealthComponent::Health）是 0..1，
	 * 显示口径是 0..MaxHealthPlaceholder（=100），两者只在这里换算。
	 * 需求写明「暂时还没有扣血，就占位用」，所以读不到血量时回落成这个满血值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|HUD")
	float MaxHealthPlaceholder = 100.f;

	/** 当前该显示的血量（0..MaxHealthPlaceholder 的整数值）。拿不到玩家血量时是占位满血。 */
	UFUNCTION(BlueprintPure, Category = "FPS|HUD")
	int32 GetDisplayedHealth() const;

	/** 把弹药、手雷、载具三块读进控件。没有 Pawn 时不动控件。 */
	UFUNCTION(BlueprintCallable, Category = "FPS|HUD")
	void Refresh();

	/** 当前显示的时速（uu/s 换算成 km/h）。没有载具时是 0。 */
	UFUNCTION(BlueprintPure, Category = "FPS|HUD")
	float GetDisplayedSpeedKmh() const;

	/** 车是不是在附近/在开。HUD 用它决定显示「按 E 上车」还是「按 E 下车」。 */
	UFUNCTION(BlueprintPure, Category = "FPS|HUD")
	bool IsInVehicle() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_Health;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_Ammo;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_Grenade;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_VehicleName;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_EnterPrompt;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> txt_VehicleSpeed;

private:
	/** 玩家 Pawn。开载具时控制器占有的是车，所以这里可能拿到 ASimpleVehiclePawn。 */
	APawn* PlayerPawn() const;

	/**
	 * 显示要用的 AFpsPawn：本人是 FpsPawn 就用本人；在车上的话按司机回读。
	 * 弹药/手雷与血量值都走这里，保证「人在车上时仍读人的数据」口径一致。
	 */
	const AFpsPawn* FpsPawnForDisplay() const;

	/** 场景里离玩家最近的一台载具（400uu 内），没有就 nullptr。 */
	ASimpleVehiclePawn* NearestVehicle() const;

	void SetTextSafe(TObjectPtr<UTextBlock>& Block, const FString& Text) const;

	FTimerHandle RefreshTimer;
};
