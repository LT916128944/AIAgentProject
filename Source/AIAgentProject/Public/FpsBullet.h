#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FpsBullet.generated.h"

class UStaticMeshComponent;

/** 看得见的弹体。不负责伤害，飞到落点就销毁。 */
UCLASS()
class AIAGENTPROJECT_API AFpsBullet : public AActor
{
	GENERATED_BODY()

public:
	AFpsBullet();

	void Launch(const FVector& From, const FVector& To);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "FPS")
	TObjectPtr<UStaticMeshComponent> Body;

	FVector Direction = FVector::ForwardVector;
	float Remaining = 0.f;

	static constexpr float Speed = 900.f;
};
