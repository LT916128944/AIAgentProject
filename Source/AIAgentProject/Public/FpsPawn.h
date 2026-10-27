#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FpsPawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	int32 HitCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UCapsuleComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UCameraComponent> ViewCamera;

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void Fire();

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void Drive(float Forward, float Right, float DeltaSeconds);

protected:
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

	static constexpr float TraceRange = 10000.f;
};
