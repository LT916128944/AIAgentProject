#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlatformProgressComponent.generated.h"

class APlatformProgressActor;

// HUD 绑定用：金币计数变化。参数是当前已收集数
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlatformCoinCollectedSignature, int32, CollectedCount);
// 收齐：开门 / 通关显示
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlatformAllCollectedSignature);
// 倒计时归零且没收齐：失败分支
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlatformTimeOutSignature);

/**
 * 平台跳跃关卡进度组件：金币计数与倒计时。玩法逻辑在组件里，Actor 只留生成与挂载。
 * 计数 倒计时 通关 失败
 */
UCLASS(ClassGroup = (Platform), meta = (BlueprintSpawnableComponent))
class AIAGENTPROJECT_API UPlatformProgressComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPlatformProgressComponent();

    // 需要收集的金币总数。金币在 BeginPlay 里自注册（RegisterPendingCoin），关卡里放了多少就是多少
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    int32 TotalCoins = 0;

    // 倒计时秒数。默认 90 秒
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    float CountdownSeconds = 90.f;

    // 已收集数。只由 NotifyCoinCollected 递增
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    int32 CollectedCount = 0;

    // 剩余秒数。只读
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    float RemainingSeconds = 0.f;

    // 是否已收齐
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    bool bAllCollected = false;

    // 倒计时是否已归零（失败分支）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    bool bTimeOut = false;

    // HUD 绑定口
    UPROPERTY(BlueprintAssignable, Category = "Platform")
    FPlatformCoinCollectedSignature OnCoinCollectedDyn;

    UPROPERTY(BlueprintAssignable, Category = "Platform")
    FPlatformAllCollectedSignature OnAllCollectedDyn;

    UPROPERTY(BlueprintAssignable, Category = "Platform")
    FPlatformTimeOutSignature OnTimeOutDyn;

    // 金币自注册：关卡里多一枚金币，总数多一。开场前调用
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void RegisterPendingCoin();

    // 收到一枚金币。到 TopCoins 归零只当裁判，不移动门——门自己绑 OnAllCollectedDyn
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void NotifyCoinCollected();

    // 开始倒计时。BeginPlay 会自动调
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void StartCountdown();

    // 重置进度并重启倒计时。人配好了总数时用这把钥匙改配置，不复用持久字段
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void ResetProgress(int32 InTotalCoins, float InCountdownSeconds);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // 表现事件，图由人接：音效、粒子、UI 特效
    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnCoinCollected();

    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnAllCollected();

    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnTimeOut();

protected:
    virtual void BeginPlay() override;

private:
    bool bCountdownRunning = false;

    void BroadcastAllCollected();
};

/**
 * 进度中枢 Actor：关卡里放一个。金币和门通过它对上。
 * level 进度 中枢
 */
UCLASS()
class AIAGENTPROJECT_API APlatformProgressActor : public AActor
{
    GENERATED_BODY()

public:
    APlatformProgressActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
    TObjectPtr<UPlatformProgressComponent> Progress;

    UFUNCTION(BlueprintPure, Category = "Platform")
    UPlatformProgressComponent* GetProgress() const { return Progress; }

    // 全关卡唯一的进度组件查找器。零个或多个时返回空指针，调用方不猜
    static UPlatformProgressComponent* FindWorldProgress(UWorld* World);
};