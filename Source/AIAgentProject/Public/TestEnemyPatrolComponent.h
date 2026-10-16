#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestEnemyPatrolComponent.generated.h"

/**
 * 敌人巡逻组件。绕出生点巡逻，发现玩家后追击，进入近战距离时通知蓝图。
 * 巡逻 追击 近战
 */
UCLASS(ClassGroup = (Test), meta = (BlueprintSpawnableComponent))
class AIAGENTPROJECT_API UTestEnemyPatrolComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTestEnemyPatrolComponent();

    // 巡逻半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float PatrolRadius;

    // 移动速度，uu/s
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float MoveSpeed;

    // 是否启用追击
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bEnableChase;

    // 发现玩家的距离
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float DetectRadius;

    // 近战距离
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float AttackRange;

    // 近战间隔，秒
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float AttackInterval;

    // 近战调用次数。只记次数，不播动画，不造成伤害
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    int32 MeleeAttackCount;

    // 开始巡逻
    UFUNCTION(BlueprintCallable, Category = "Test")
    void StartPatrol();

    // 进入巡逻，图由人接
    UFUNCTION(BlueprintImplementableEvent, Category = "Test")
    void OnPatrol();

    // 进入追击，图由人接
    UFUNCTION(BlueprintImplementableEvent, Category = "Test")
    void OnChase();

    // 近战攻击，图由人接。不造成伤害，不播动画
    UFUNCTION(BlueprintImplementableEvent, Category = "Test")
    void OnMeleeAttack();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
    virtual void BeginPlay() override;

private:
    void PickPatrolTarget();
    void MoveToward(const FVector& Target, float DeltaTime);

    FVector HomeLocation;
    FVector PatrolTarget;
    float PatrolAngle;
    float LastAttackTime;
    bool bChasing;
};
