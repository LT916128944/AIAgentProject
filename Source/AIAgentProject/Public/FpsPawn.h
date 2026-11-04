#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FpsPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class UFpsHealthComponent;
class UUserWidget;

/**
 * 简易第一人称射击 Pawn。WASD 与鼠标口径仿 ATestPawn。
 * 不自动占据玩家，避免和平台跳跃的 ATestPawn 抢控制权。
 */
UCLASS()
class AIAGENTPROJECT_API AFpsPawn : public APawn
{
	GENERATED_BODY()

public:
	AFpsPawn();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float LookSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	int32 Ammo = 12;

	// 手榴弹库存。拾取物加，ThrowGrenade 扣
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	int32 GrenadeCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float ThrowSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float ThrowUpBias = 300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	int32 HitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UCapsuleComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UCameraComponent> ViewCamera;

	/**
	 * 玩家血量真源，复用靶子的 UFpsHealthComponent（口径 0..1，满血 1）。
	 * 只提供数据来源，本任务不加扣血触发；HUD 读 GetHealth01 再乘显示上限。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Health")
	TObjectPtr<UFpsHealthComponent> Health;

	/** 血量读数，0..1（组件里就是这么存的，这里不改口径）。没有组件时按满血 1 返回。 */
	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	float GetHealth01() const;

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void Fire();

	// 投手榴弹：没库存就不动。从相机位置沿视线抛 AFpsGrenadeProjectile
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void ThrowGrenade();

	// 拾取物加库存的唯一入口
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void AddGrenades(int32 Count);

	// 按 E 找 400uu 内最近未占用载具上车；载具上再按 E 下车（切回本 Pawn）
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void TryEnterVehicle();

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void Drive(float Forward, float Right, float DeltaSeconds);

	/**
	 * 把 HUD 控件挂进视口（20261029-002746）。默认类 /Game/UI/WBP_FpsHUD，
	 * 可在蓝图/关卡实例里换成别的 WBP。没有玩家控制器或世界没有视口时不建控件。
	 * 幂等：已经挂过就直接返回。
	 */
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void EnsureHud();

	/** 这次要挂的 HUD 控件类。默认 WBP_FpsHUD（软引用，不硬加载）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|HUD")
	TSoftClassPtr<UUserWidget> HudWidgetClass;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void OnMoveKeyPressed(FKey Key);
	void OnMoveKeyReleased(FKey Key);
	void OnMoveForwardAxis(float Value);
	void OnMoveRightAxis(float Value);
	void OnTurnAxis(float Value);
	void OnLookUpAxis(float Value);
	void SetMoveKey(const FKey& Key, float Value);
	void ApplyLocalMove(float Forward, float Right, float DeltaSeconds);
	void ShowBullet(const FVector& From, const FVector& To);

	float AxisW = 0.f;
	float AxisS = 0.f;
	float AxisA = 0.f;
	float AxisD = 0.f;
	float AxisForward = 0.f;
	float AxisRight = 0.f;

	// 已挂上的 HUD 实例。只由 EnsureHud 写，避免每次 BeginPlay/Possess 重复 AddToViewport。
	UPROPERTY()
	TObjectPtr<UUserWidget> HudInstance;

	// WBP 还没做出来时置位：Tick 不再每帧 LoadSynchronous 重试。
	bool bHudClassMissing = false;

	static constexpr float TraceRange = 10000.f;
};
