#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformCoin.generated.h"

class URotatingMovementComponent;
class USphereComponent;
class UPlatformProgressComponent;

/**
 * 旋转金币。旋转由引擎 RotatingMovementComponent 完成（不改图）；
 * 拾取每帧按距离判定（碰撞通道被蓝图覆盖成 Block/NoCollision 时重叠事件会静默不触发），
 * 收走后上报进度中枢。拾取表现 OnCollected 图由人接。
 * 金币 拾取 旋转
 */
UCLASS()
class AIAGENTPROJECT_API ACoin : public AActor
{
    GENERATED_BODY()

public:
    ACoin();

    // 旋转组件。默认绕 Z 轴每秒 180 度
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    TObjectPtr<URotatingMovementComponent> Rotating;

    // 拾取判定球。半径默认 100 uu（1 米）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    TObjectPtr<USphereComponent> PickupSphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    float PickupRadius = 100.f;

    // 拾取入口。真实重叠和自动化模拟都走这一条路：置已收集、上报进度、触发表现
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void NotifyCollected(AActor* Collector);

    UFUNCTION(BlueprintPure, Category = "Platform")
    bool IsCollected() const { return bCollected; }

    // 拾取表现，图由人接：音效、粒子、飘字。不在 C++ 里播
    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnCollected();

protected:
    virtual void BeginPlay() override;

private:
    // 拾取判定只走距离这一条：碰撞通道在蓝图侧容易被覆盖成 Block/NoCollision，
    // 重叠事件会静默不触发（20261021 实测：金币与 Pawn 深度重叠却不收）；
    // 而重叠事件的有效距离是「球半径 + 胶囊半径」，比 PickupRadius 大一圈，
    // 两条路一起用会把相邻金币一起收掉。所以只留距离判定。
    virtual void Tick(float DeltaSeconds) override;

    AActor* FindPlayerPawn() const;

    bool bCollected = false;
};