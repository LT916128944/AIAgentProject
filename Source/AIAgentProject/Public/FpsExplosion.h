#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FpsExplosion.generated.h"

class UStaticMeshComponent;

/** 爆炸的兜底表现：一个膨胀球 + 八个碎片小球飞散缩小。零资产，NS_Explosion 缺失时才用。 */
UCLASS()
class AIAGENTPROJECT_API AFpsExplosion : public AActor
{
	GENERATED_BODY()

public:
	AFpsExplosion();

	virtual void Tick(float DeltaSeconds) override;

private:
	// 膨胀球：前半程 0.1 缩到 2.0，后半程随总寿命缩回 0
	UPROPERTY(VisibleAnywhere, Category = "FPS")
	TObjectPtr<UStaticMeshComponent> Blast;

	UPROPERTY(VisibleAnywhere, Category = "FPS")
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	TArray<FVector> ShardDirections;

	float Age = 0.f;

	static constexpr float LifeTime = 0.6f;
	static constexpr float BlastPeakScale = 2.0f;
	static constexpr float ShardBaseScale = 0.15f;
};
