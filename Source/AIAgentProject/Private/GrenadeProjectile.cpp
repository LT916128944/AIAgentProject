#include "GrenadeProjectile.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AGrenadeProjectile::AGrenadeProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(8.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    RootComponent = Collision;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = Collision;
    ProjectileMovement->InitialSpeed = 0.f;
    ProjectileMovement->MaxSpeed = 3000.f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.2f;
    ProjectileMovement->Friction = 0.5f;
    ProjectileMovement->ProjectileGravityScale = 1.f;
    Collision->OnComponentHit.AddDynamic(this, &AGrenadeProjectile::OnProjectileHit);
}

void AGrenadeProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (AActor* GrenadeOwner = GetOwner())
    {
        Collision->IgnoreActorWhenMoving(GrenadeOwner, true);
    }
    SetLifeSpan(12.f);
}

void AGrenadeProjectile::SetThrowVelocity(const FVector& Velocity)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = Velocity;
        ProjectileMovement->Activate(true);
    }
}

void AGrenadeProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (bExploded || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }
    Explode();
}

void AGrenadeProjectile::Explode()
{
    if (bExploded)
    {
        return;
    }
    bExploded = true;
    const FVector Location = GetActorLocation();
    if (UWorld* World = GetWorld())
    {
        TArray<AActor*> IgnoreActors;
        if (!bDamageSelf)
        {
            IgnoreActors.Add(this);
        }
        UGameplayStatics::ApplyRadialDamageWithFalloff(World, BaseDamage, MinimumDamage, Location, ExplosionRadius * 0.25f, ExplosionRadius, 1.f, UDamageType::StaticClass(), IgnoreActors, this, GetInstigatorController(), ECC_Visibility);
        // UGameplayStatics 没有 ApplyRadialImpulse，改为对范围内模拟物理的图元逐个施加径向冲量
        TArray<FOverlapResult> Overlaps;
        const FCollisionShape ImpulseShape = FCollisionShape::MakeSphere(ExplosionRadius);
        FCollisionQueryParams ImpulseParams(SCENE_QUERY_STAT(GrenadeExplode), false, this);
        if (World->OverlapMultiByChannel(Overlaps, Location, FQuat::Identity, ECC_WorldDynamic, ImpulseShape, ImpulseParams))
        {
            TArray<UPrimitiveComponent*> ImpulsedComponents;
            for (const FOverlapResult& Overlap : Overlaps)
            {
                UPrimitiveComponent* OverlapComponent = Overlap.GetComponent();
                if (!OverlapComponent || ImpulsedComponents.Contains(OverlapComponent))
                {
                    continue;
                }
                ImpulsedComponents.Add(OverlapComponent);
                if (OverlapComponent->IsSimulatingPhysics())
                {
                    OverlapComponent->AddRadialImpulse(Location, ExplosionRadius, ImpulseStrength, ERadialImpulseFalloff::RIF_Linear, true);
                }
            }
        }
        DrawDebugSphere(World, Location, ExplosionRadius, 32, FColor::Orange, false, 1.2f, 0, 2.f);
        FActorSpawnParameters LightParams;
        LightParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APointLight* Light = World->SpawnActor<APointLight>(Location, FRotator::ZeroRotator, LightParams);
        if (Light)
        {
            Light->SetLifeSpan(0.25f);
            if (UPointLightComponent* LightComponent = Light->FindComponentByClass<UPointLightComponent>())
            {
                LightComponent->SetIntensity(5000.f);
                LightComponent->SetLightColor(FLinearColor(1.f, 0.45f, 0.05f));
                LightComponent->SetAttenuationRadius(ExplosionRadius * 1.5f);
            }
        }
    }
    if (Mesh)
    {
        Mesh->SetVisibility(false);
    }
    if (Collision)
    {
        Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (ProjectileMovement)
    {
        ProjectileMovement->StopMovementImmediately();
    }
    SetLifeSpan(0.2f);
}
