#include "GrenadePickup.h"
#include "GrenadeProjectile.h"
#include "Components/InputComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AGrenadePickup::AGrenadePickup()
{
    PrimaryActorTick.bCanEverTick = false;
    PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
    PickupSphere->InitSphereRadius(50.f);
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    RootComponent = PickupSphere;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(PickupSphere);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AGrenadePickup::OnPickupOverlap);
    ProjectileClass = AGrenadeProjectile::StaticClass();
}

void AGrenadePickup::BeginPlay()
{
    Super::BeginPlay();
}

bool AGrenadePickup::Pickup(AActor* NewHolder)
{
    if (!NewHolder || HeldBy.IsValid())
    {
        return false;
    }
    HeldBy = NewHolder;
    PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AttachToActor(NewHolder, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    return true;
}

AGrenadeProjectile* AGrenadePickup::ThrowHeld(const FVector& ThrowDirection, float ThrowForce)
{
    if (!HeldBy.IsValid() || !ProjectileClass)
    {
        return nullptr;
    }
    AActor* Holder = HeldBy.Get();
    HeldBy.Reset();
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    const FVector Direction = ThrowDirection.GetSafeNormal();
    const FVector SpawnLocation = Holder->GetActorLocation() + Direction * SpawnForwardOffset;
    const FRotator SpawnRotation = Direction.Rotation();
    FActorSpawnParameters Params;
    Params.Owner = Holder;
    Params.Instigator = Cast<APawn>(Holder);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AGrenadeProjectile* Projectile = GetWorld()->SpawnActor<AGrenadeProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, Params);
    if (Projectile)
    {
        const float Force = ThrowForce > 0.f ? ThrowForce : ThrowSpeed;
        Projectile->SetThrowVelocity(Direction * Force);
    }
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    SetLifeSpan(0.1f);
    return Projectile;
}

void AGrenadePickup::BindThrowInput(APawn* Holder, FName ActionName)
{
    if (Holder && Holder->InputComponent)
    {
        Holder->InputComponent->BindAction(ActionName, IE_Pressed, this, &AGrenadePickup::ThrowHeldFromInput);
    }
}

void AGrenadePickup::ThrowHeldFromInput()
{
    if (HeldBy.IsValid())
    {
        AActor* Holder = HeldBy.Get();
        ThrowHeld(Holder->GetActorForwardVector(), ThrowSpeed);
    }
}

void AGrenadePickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HeldBy.IsValid() && OtherActor && OtherActor->IsA<APawn>())
    {
        Pickup(OtherActor);
    }
}
