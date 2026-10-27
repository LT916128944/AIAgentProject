#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FpsHealthComponent.generated.h"

/** 射击靶的血量。表现用 OnDamaged，图可以不接。 */
UCLASS(ClassGroup = (FPS), meta = (BlueprintSpawnableComponent))
class AIAGENTPROJECT_API UFpsHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS")
	float Health = 1.f;

	UFUNCTION(BlueprintCallable, Category = "FPS")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category = "FPS")
	bool IsDepleted() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPS")
	void OnDamaged(float Remaining);
};
