#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TestAIController.generated.h"

/**
 * 测试 AI 控制器，负责控制 Pawn 的 AI 行为。
 * AI 控制 人工智能 controller
 */
UCLASS()
class AIAGENTPROJECT_API ATestAIController : public AAIController
{
    GENERATED_BODY()

public:
    ATestAIController();

    // 巡逻半径
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float PatrolRadius;

    // 是否启用追击
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bEnableChase;

    // 开始巡逻
    UFUNCTION(BlueprintCallable, Category = "Test")
    void StartPatrol();
};