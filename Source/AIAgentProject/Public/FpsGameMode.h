#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FpsGameMode.generated.h"

/** 只给射击关用的模式。默认 Pawn 是 AFpsPawn，不替换项目全局 GameMode。 */
UCLASS()
class AIAGENTPROJECT_API AFpsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFpsGameMode();
};
