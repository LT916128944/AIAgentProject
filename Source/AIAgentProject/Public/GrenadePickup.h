#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadePickup.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UPrimitiveComponent;
class AGrenadeProjectile;
class APawn;

UCLASS()
class AGrenadePickup : public AActor
{
    GENERATED_BODY()
public:
    AGrenadePickup();
    UFUNCTION(BlueprintCallable, Category="Grenade")
    bool Pickup(AActor* NewHolder);
    UFUNCTION(BlueprintCallable, Category="Grenade")
    AGrenadeProjectile* ThrowHeld(const FVector& ThrowDirection, float ThrowForce);
    UFUNCTION(BlueprintCallable, Category="Grenade")
    void BindThrowInput(APawn* Holder, FName ActionName);
    UFUNCTION(BlueprintCallable, Category="Grenade")
    bool IsHeld() const { return HeldBy.IsValid(); }
protected:
    virtual void BeginPlay() override;
    UFUNCTION()
    void OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    UFUNCTION()
    void ThrowHeldFromInput();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    TObjectPtr<USphereComponent> PickupSphere;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    TSubclassOf<AGrenadeProjectile> ProjectileClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float ThrowSpeed = 1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float SpawnForwardOffset = 60.f;
    UPROPERTY()
    TWeakObjectPtr<AActor> HeldBy;
};
