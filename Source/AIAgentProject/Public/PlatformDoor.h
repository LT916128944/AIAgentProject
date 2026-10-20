#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformDoor.generated.h"

class UPlatformProgressComponent;

/**
 * 平台跳跃终点门。BeginPlay 绑进度中枢的 OnAllCollectedDyn，收齐后整体位移 OpenOffset。
 * 开门动画、音效由 OnOpened 图由人接。
 * 门 开门 收齐
 */
UCLASS()
class AIAGENTPROJECT_API APlatformDoor : public AActor
{
    GENERATED_BODY()

public:
    APlatformDoor();

    // 开门位移。默认升高 300 uu
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
    FVector OpenOffset = FVector(0.f, 0.f, 300.f);

    // 绑进度中枢。零个或多个时不绑：门保持关，不猜
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void BindToProgress(UPlatformProgressComponent* InProgress);

    // 开门。已开过就不再动
    UFUNCTION(BlueprintCallable, Category = "Platform")
    void Open();

    UFUNCTION(BlueprintPure, Category = "Platform")
    bool IsOpen() const { return bIsOpen; }

    // 开门表现，图由人接：动画、音效、粒子
    UFUNCTION(BlueprintImplementableEvent, Category = "Platform")
    void OnOpened();

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleAllCollected();

    bool bIsOpen = false;
};