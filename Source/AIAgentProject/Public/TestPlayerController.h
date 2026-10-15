#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TestPlayerController.generated.h"

/**
 * 测试玩家控制器，负责接收玩家输入。
 * 玩家控制器 输入 input controller
 */
UCLASS()
class AIAGENTPROJECT_API ATestPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATestPlayerController();

    // 是否显示鼠标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bTestShowMouseCursor;

    // 处理输入
    UFUNCTION(BlueprintCallable, Category = "Test")
    void HandleInput();

    virtual void OnPossess(APawn* InPawn) override;
};