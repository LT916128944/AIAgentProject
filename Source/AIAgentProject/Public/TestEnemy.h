#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestEnemyPatrolComponent.h"
#include "TestEnemy.generated.h"

/** 敌人行为状态 */
UENUM(BlueprintType)
enum class ETestEnemyState : uint8
{
    Patrol UMETA(DisplayName = "Patrol"),
    Chase  UMETA(DisplayName = "Chase"),
    Attack UMETA(DisplayName = "Attack")
};

/**
 * 测试敌人。只生成并挂上巡逻组件，行为在组件里。
 * 敌人 enemy
 */
UCLASS()
class AIAGENTPROJECT_API ATestEnemy : public AActor
{
    GENERATED_BODY()

public:
    ATestEnemy();

    // 打一行日志，用来确认新代码已被编辑器加载
    UFUNCTION(BlueprintCallable, Category = "Test")
    void PrintTestLog();

    // 初始化：设置巡逻参数并拉起组件
    virtual void BeginPlay() override;

    // 从蓝图里配置巡逻半径 / 巡逻速度，并启动巡逻
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void InitPatrol(float InPatrolRadius = 800.0f, float InMoveSpeed = 300.0f);

    // 巡逻组件
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    TObjectPtr<UTestEnemyPatrolComponent> PatrolComponent;

    // 是否能发现玩家（可被蓝图开关）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    bool bEnableDetection = true;

    // ===== 行为参数 =====

    /** 发现玩家的距离（厘米） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float DetectionRange = 1500.0f;

    /** 追击时保持的距离（厘米），进入该距离即停下 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float ChaseStopRange = 200.0f;

    /** 进入该距离即可攻击（厘米） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float AttackRange = 250.0f;

    /** 单次攻击造成的伤害 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float AttackDamage = 10.0f;

    /** 攻击冷却（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float AttackCooldown = 1.5f;

    /** 追击移动速度（厘米/秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float ChaseSpeed = 400.0f;

    /** 玩家超出该距离（DetectionRange 的倍率）后丢失目标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|AI")
    float LoseTargetRangeMultiplier = 1.5f;

    // ===== 状态 =====

    /** 当前行为状态 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test|AI")
    ETestEnemyState CurrentState = ETestEnemyState::Patrol;

    /** 当前锁定的玩家 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test|AI")
    TObjectPtr<ATestPawn> TargetPawn;

    /** 距下次可攻击的剩余时间 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test|AI")
    float AttackCooldownRemaining = 0.0f;

    // ===== 行为接口 =====

    /** 每帧驱动状态机：巡逻 -> 发现追击 -> 近距离攻击 */
    virtual void Tick(float DeltaSeconds) override;

    /** 在关卡中寻找最近的玩家（返回 nullptr 表示没有） */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    ATestPawn* FindNearestPlayer() const;

    /** 巡逻时的处理 */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void TickPatrol(float DeltaSeconds);

    /** 追击目标 */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void TickChase(float DeltaSeconds, ATestPawn* Player);

    /** 近距离攻击目标 */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void TickAttack(float DeltaSeconds, ATestPawn* Player);

    /** 切换行为状态 */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void SetState(ETestEnemyState NewState);

    /** 对目标施加一次攻击 */
    UFUNCTION(BlueprintCallable, Category = "Test|AI")
    void ApplyAttack(ATestPawn* Player);

    // ===== 查询接口（供蓝图 / UI 使用） =====

    /** 当前是否在巡逻状态 */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    bool IsPatrolling() const { return CurrentState == ETestEnemyState::Patrol; }

    /** 当前是否在追击状态 */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    bool IsChasing() const { return CurrentState == ETestEnemyState::Chase; }

    /** 当前是否在攻击状态 */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    bool IsAttacking() const { return CurrentState == ETestEnemyState::Attack; }

    /** 当前锁定的目标（可能为空） */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    ATestPawn* GetTargetPawn() const { return TargetPawn; }

    /** 与目标的水平距离，没有目标时返回 -1 */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    float GetDistanceToTarget() const;

    /** 当前是否有有效目标（供蓝图分支使用） */
    UFUNCTION(BlueprintPure, Category = "Test|AI")
    bool HasTarget() const;
};
