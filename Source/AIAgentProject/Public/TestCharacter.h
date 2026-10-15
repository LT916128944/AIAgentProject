#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TestCharacter.generated.h"

/**
 * 测试角色，带骨骼网格与移动组件。
 * 玩家 人物 character player
 */
UCLASS()
class AIAGENTPROJECT_API ATestCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ATestCharacter();

    // 血量 health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float Health;

    // 是否存活 alive
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bIsAlive;

    // 跳跃
    UFUNCTION(BlueprintCallable, Category = "Test")
    void JumpAction();

    // 攻击
    UFUNCTION(BlueprintCallable, Category = "Test")
    void TestAttack();
};