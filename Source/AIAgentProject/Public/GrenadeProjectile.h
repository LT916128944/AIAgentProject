#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadeProjectile.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;

UCLASS()
class AGrenadeProjectile : public AActor
{
    GENERATED_BODY()
public:
    AGrenadeProjectile();
    UFUNCTION(BlueprintCallable, Category="Grenade")
    void SetThrowVelocity(const FVector& Velocity);
protected:
    virtual void BeginPlay() override;
    UFUNCTION()
    void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
    void Explode();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float ExplosionRadius = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float BaseDamage = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float MinimumDamage = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    float ImpulseStrength = 800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grenade")
    bool bDamageSelf = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grenade")
    bool bExploded = false;
};
