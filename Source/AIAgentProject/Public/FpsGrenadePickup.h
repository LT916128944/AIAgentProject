#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FpsGrenadePickup.generated.h"

class URotatingMovementComponent;
class UStaticMeshComponent;

/** 场景里的手榴弹拾取物。拾取判定仿 ACoin：只走每帧距离判定，重叠事件不可靠。 */
UCLASS()
class AIAGENTPROJECT_API AFpsGrenadePickup : public AActor
{
	GENERATED_BODY()

public:
	AFpsGrenadePickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<URotatingMovementComponent> Rotating;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UStaticMeshComponent> Body;

	// 拾取判定半径，默认 100 uu（1 米）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float PickupRadius = 100.f;

	// 拾取一次给几颗
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	int32 GrantCount = 3;

	// 拾取入口。真实距离判定与自动化模拟都走这一条路：加弹、触发表现、自毁
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void NotifyCollected(AActor* Collector);

	UFUNCTION(BlueprintPure, Category = "FPS")
	bool IsCollected() const { return bCollected; }

	// 拾取表现，图由人接：音效、粒子、飘字。不在 C++ 里播
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS")
	void OnCollected();

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	AActor* FindPlayerPawn() const;

	bool bCollected = false;
};
