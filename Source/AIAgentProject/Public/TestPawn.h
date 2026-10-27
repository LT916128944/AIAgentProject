#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TestPawn.generated.h"

class APlayerController;
class FTestPawnLookTest;
class FTestPawnWasdTest;
class UCapsuleComponent;
class UPlatformHUDWidget;
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

    // 移动速度，单位 uu/s。默认 300（3 米/秒），平台间距按这个速度设计
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

    // 碰撞胶囊。金币的球形重叠要认 Pawn，没有碰撞体就永远不触发拾取
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    TObjectPtr<UCapsuleComponent> Collision;

    // 平台跳跃 HUD。留空则不创建；默认指向 /Game/UI/WBP_HUD，父类是 UPlatformHUDWidget
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    TSoftClassPtr<UPlatformHUDWidget> HudWidgetClass;

    // 创建并挂上 HUD。已挂过就返回。没有控制器或蓝图时什么都不做，不崩。
    UFUNCTION(BlueprintCallable, Category = "Test")
    void EnsureHud();

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
    // 起跳时的脚底高度，仅做记录用；落地不再按它夹，改由地面检测决定
    float JumpStartZ = 0.f;
    bool bJumping = false;

    // 每帧更新跳跃位移。下落时向下做地面检测，落到脚下支撑面（平台表面）
    void UpdateJump(float DeltaSeconds);

    // 向下探测脚下支撑面。TraceLength 是自脚底向下额外探测的距离（uu）
    // 命中返回 true，OutGroundZ 为支撑面的世界 Z（落到它上面，而不是回到起跳高度）
    bool FindGroundBelow(float TraceLength, float& OutGroundZ) const;

    // 地面检测在落点以下额外多探的距离（uu），避免刚好落在面内漏检
    static constexpr float GroundTraceExtra = 5.f;

    void OnMoveKeyPressed(FKey Key);
    void OnMoveKeyReleased(FKey Key);
    void OnMoveForwardAxis(float Value);
    void OnMoveRightAxis(float Value);
    void SetMoveKey(const FKey& Key, float Value);
    void ApplyLocalMove(float Forward, float Right, float Up, float DeltaSeconds);

    UPROPERTY()
    TObjectPtr<AActor> HeldActor;

    // 已挂上的 HUD 实例。只由 EnsureHud 写
    UPROPERTY()
    TObjectPtr<UPlatformHUDWidget> HudInstance;

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