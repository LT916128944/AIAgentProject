#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FpsTarget.generated.h"

class UBoxComponent;
class UFpsHealthComponent;
class UStaticMeshComponent;

/** 简易靶。看得见的身体是方块，血量在 UFpsHealthComponent，耗尽后整只隐藏。 */
UCLASS()
class AIAGENTPROJECT_API AFpsTarget : public AActor
{
	GENERATED_BODY()

public:
	AFpsTarget();

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void ReactToHit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UFpsHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UBoxComponent> Body;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
