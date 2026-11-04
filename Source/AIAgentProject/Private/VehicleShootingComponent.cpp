#include "VehicleShootingComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"

UVehicleShootingComponent::UVehicleShootingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	FireCooldown = 0.25f;
	TraceDistance = 5000.f;
	HitDamage = 20.f;

	MuzzleEffect = nullptr;
	HitEffect = nullptr;
	FireSound = nullptr;

	bInVehicleOccupancy = false;
	bLastShotHit = false;
	LastFireTime = -1000.f;
	LastHitLocation = FVector::ZeroVector;
}

void UVehicleShootingComponent::BeginPlay()
{
	Super::BeginPlay();
	LastFireTime = -1000.f;
	bLastShotHit = false;
	LastHitActor = nullptr;
	LastHitLocation = FVector::ZeroVector;
}

void UVehicleShootingComponent::SetVehicleOccupancy(bool bOccupied)
{
	bInVehicleOccupancy = bOccupied;

	if (!bInVehicleOccupancy)
	{
		bLastShotHit = false;
		LastHitActor = nullptr;
	}
}

bool UVehicleShootingComponent::IsInVehicleOccupancy() const
{
	return bInVehicleOccupancy;
}

bool UVehicleShootingComponent::WasLastShotAHit() const
{
	return bLastShotHit;
}

AActor* UVehicleShootingComponent::GetLastHitActor() const
{
	return LastHitActor.Get();
}

FVector UVehicleShootingComponent::GetLastHitLocation() const
{
	return LastHitLocation;
}

bool UVehicleShootingComponent::TryFire()
{
	if (!bInVehicleOccupancy)
	{
		return false;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		return false;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastFireTime < FireCooldown)
	{
		return false;
	}
	LastFireTime = Now;

	FVector ViewLocation = Owner->GetActorLocation();
	FRotator ViewRotation = Owner->GetActorRotation();
	AController* InstigatorController = nullptr;

	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		if (AController* OwnerController = OwnerPawn->GetController())
		{
			InstigatorController = OwnerController;
			OwnerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
	}

	const FVector ShotDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ShotDirection * TraceDistance;

	FCollisionQueryParams QueryParams(TEXT("VehicleShootingTrace"), false, Owner);
	QueryParams.AddIgnoredActor(Owner);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);

	bLastShotHit = bHit;
	LastHitActor = bHit ? Hit.GetActor() : nullptr;
	LastHitLocation = bHit ? Hit.ImpactPoint : TraceEnd;

	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, MuzzleEffect, ViewLocation, ViewRotation);
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, ViewLocation);
	}

	if (bHit)
	{
		if (HitEffect)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, HitEffect, Hit.ImpactPoint, (-ShotDirection).Rotation());
		}

		if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyPointDamage(HitActor, HitDamage, ShotDirection, Hit, InstigatorController, Owner, UDamageType::StaticClass());
		}
	}

	return true;
}
