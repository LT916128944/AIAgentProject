#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TestPawn.generated.h"

class APlayerController;
class FTestPawnLookTest;
class FTestPawnWasdTest;
class UCameraComponent;
class USpringArmComponent;

/**
 * 测试 Pawn，可被 Controller 控制，代表可移动单位。
 * 单位 角色载体 pawn
 */
UCLASS()
class AIAGENTPROJECT_API ATestPawn : public APawn
{
    GENERATED_BODY()

public:
    ATestPawn();

    virtual void Tick(float DeltaSeconds) override;

    // 移动速度，单位 uu/s。默认 100（1 米/秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float MoveSpeed;

    // 是否可被 AI 控制
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bCanBePossessed;

    // 向前移动。Amount 是秒数，1 表示按住前进键 1 秒的距离
    UFUNCTION(BlueprintCallable, Category = "Test")
    void TestMoveForward(float Amount);

    // 空格跳跃：跳跃高度由 JumpHeight（默认 1 米）决定
    UFUNCTION(BlueprintCallable, Category = "Test")
    void Jump();

    // 是否正在跳跃
    UFUNCTION(BlueprintPure, Category = "Test")
    bool IsJumping() const;

    friend class FTestPawnWasdTest;
    friend class FTestPawnLookTest;

    // 左键切换拾取：1 米内 Tag=InteractableActor 的最近 Actor；再按一次回到原位
    UFUNCTION(BlueprintCallable, Category = "Test")
    void TogglePickup();

    // 跟随相机，挂在弹簧臂末端
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    TObjectPtr<UCameraComponent> FollowCamera;

protected:
    virtual void BeginPlay() override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void PawnClientRestart() override;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // 鼠标移动输入的旋转处理：X = Yaw，Y = Pitch
    void OnTurnAxis(float Value);
    void OnLookUpAxis(float Value);

    // 镜头旋转灵敏度（度/输入单位）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float LookSensitivity = 1.f;

    // 跳跃高度，单位 uu。默认 100（1 米）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float JumpHeight = 100.f;

    void AlignFollowCamera();
    void UseThisAsViewTarget(APlayerController* Player);
    void ClaimPlayerView();

    // 跳跃状态
    float VerticalVelocity = 0.f;
    float JumpStartZ = 0.f;
    bool bJumping = false;

    // 每帧更新跳跃位移
    void UpdateJump(float DeltaSeconds);

    void OnMoveKeyPressed(FKey Key);
    void OnMoveKeyReleased(FKey Key);
    void OnMoveForwardAxis(float Value);
    void OnMoveRightAxis(float Value);
    void SetMoveKey(const FKey& Key, float Value);
    void ApplyLocalMove(float Forward, float Right, float Up, float DeltaSeconds);

    UPROPERTY()
    TObjectPtr<AActor> HeldActor;

    FTransform HeldActorOrigin;

    float AxisW = 0.f;
    float AxisS = 0.f;
    float AxisA = 0.f;
    float AxisD = 0.f;
    float AxisForward = 0.f;
    float AxisRight = 0.f;
    float AxisQ = 0.f;
    float AxisE = 0.f;
};