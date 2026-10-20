#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformMovingPlatform.generated.h"

/**
 * 移动平台。起点 = 开场位置，终点 = 起点 + MoveDelta，按 MoveSpeed 往返。
 * 玩法在 C++：Tick 插值；平台外观、贴地由人配。
 * 平台 移动 往返
 */
UCLASS()
class AIAGENTPROJECT_API AMovingPlatform : public AActor
{
    GENERATED_BODY()

public:
    AMovingPlatform();

    // 往返位移量
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    FVector MoveDelta = FVector(300.f, 0.f, 0.f);

    // 移动速度，uu/s
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    float MoveSpeed = 100.f;

    // 开场即开始移动
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    bool bStartActive = true;

    // 当前往返位置 0..1
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    float Alpha = 0.f;

    UFUNCTION(BlueprintCallable, Category = "Platform")
    void StartMoving();

    UFUNCTION(BlueprintCallable, Category = "Platform")
    void StopMoving();

    // 仿 ATestPawn：自动化测试直接驱动 Tick（临时世界没有帧循环）
    virtual void Tick(float DeltaSeconds) override;

private:
    FVector StartLocation;
    int32 Direction = 1;
    bool bMoving = false;

protected:
    virtual void BeginPlay() override;
};