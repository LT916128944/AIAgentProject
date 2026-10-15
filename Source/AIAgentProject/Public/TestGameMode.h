#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestGameMode.generated.h"

/**
 * 测试游戏模式，定义关卡默认 Pawn 与 Controller。
 * 游戏模式 规则 gamemode
 */
UCLASS()
class AIAGENTPROJECT_API ATestGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ATestGameMode();

    // 默认 Pawn 类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    TSubclassOf<APawn> MyDefaultPawn;

    // 默认 Controller 类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    TSubclassOf<APlayerController> MyPlayerController;
};