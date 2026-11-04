#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "VehicleShootingComponent.generated.h"

class UParticleSystem;
class USoundBase;

/**
 * 车载射击组件：只有在载具占据状态下才允许开火，
 * 开火做一次可视命中检测并生成可见的枪口/命中反馈。
 */
UCLASS(ClassGroup=(Vehicle), meta=(BlueprintSpawnableComponent))
class UVehicleShootingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVehicleShootingComponent();

	virtual void BeginPlay() override;

	/** 由载具占据切换（A）调用：进入/离开载具时同步开火许可。 */
	UFUNCTION(BlueprintCallable, Category="Vehicle|Shooting")
	void SetVehicleOccupancy(bool bOccupied);

	UFUNCTION(BlueprintPure, Category="Vehicle|Shooting")
	bool IsInVehicleOccupancy() const;

	/** 开火入口：仅当处于载具占据状态且冷却结束时生效，返回是否真的开了一枪。 */
	UFUNCTION(BlueprintCallable, Category="Vehicle|Shooting")
	bool TryFire();

	UFUNCTION(BlueprintPure, Category="Vehicle|Shooting")
	bool WasLastShotAHit() const;

	UFUNCTION(BlueprintPure, Category="Vehicle|Shooting")
	AActor* GetLastHitActor() const;

	UFUNCTION(BlueprintPure, Category="Vehicle|Shooting")
	FVector GetLastHitLocation() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	float FireCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	float TraceDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	float HitDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	UParticleSystem* HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle|Shooting")
	USoundBase* FireSound;

private:
	UPROPERTY(VisibleInstanceOnly, Category="Vehicle|Shooting")
	bool bInVehicleOccupancy;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastHitActor;

	bool bLastShotHit;
	float LastFireTime;
	FVector LastHitLocation;
};
