#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformCoin.generated.h"

class URotatingMovementComponent;
class USphereComponent;
class UPlatformProgressComponent;

/**
 * 旋转金币。旋转由引擎 RotatingMovementComponent 完成（不改图）；
 * 拾取走球形重叠，收走后上报进度中枢。拾取表现 OnCollected 图由人接。
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
    UFUNCTION()
    void HandleSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                             bool bFromSweep, const FHitResult& SweepResult);

    bool bCollected = false;
};