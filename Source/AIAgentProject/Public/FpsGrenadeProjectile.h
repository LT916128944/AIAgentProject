#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FpsGrenadeProjectile.generated.h"

class UNiagaraSystem;
class UStaticMeshComponent;

/** 手榴弹投掷物：抛物线 + 引信兜底。球体扫掠被挡（落地/命中）当场爆炸；伤害判定只走距离，不用 Overlap。 */
UCLASS()
class AIAGENTPROJECT_API AFpsGrenadeProjectile : public AActor
{
	GENERATED_BODY()

public:
	AFpsGrenadeProjectile();

	void Launch(const FVector& From, const FVector& InVelocity);

	virtual void Tick(float DeltaSeconds) override;

	// 爆炸入口。真实引信与自动化模拟都走这一条路：半径内靶掉血 + 表现 + 自毁
	UFUNCTION(BlueprintCallable, Category = "FPS")
	void Explode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float BlastRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float BlastDamage = 2.f;

	// 引信秒数，只作空中兜底：扫掠被挡（落地/命中）先爆。临时世界测试靠显式驱动 Tick 累积到这里
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float FuseSeconds = 2.f;

	// 主表现资产。缺失时 Explode 走 AFpsExplosion 几何兜底，不报错
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	TSoftObjectPtr<UNiagaraSystem> ExplosionTemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS")
	TObjectPtr<UStaticMeshComponent> Body;

	UFUNCTION(BlueprintPure, Category = "FPS")
	bool HasExploded() const { return bExploded; }

private:
	void SpawnVisual(const FVector& Location);

	FVector Velocity = FVector::ZeroVector;
	float FuseAge = 0.f;
	bool bExploded = false;

	static constexpr float Gravity = 980.f;
};
