#include "MyVehiclePawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

AMyVehiclePawn::AMyVehiclePawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(80.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_Pawn);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    RootComponent = Collision;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->MaxSpeed = MoveSpeed;

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(Collision);
    InteractionSphere->InitSphereRadius(250.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AMyVehiclePawn::BeginPlay()
{
    Super::BeginPlay();
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMyVehiclePawn::OnInteractionBegin);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AMyVehiclePawn::OnInteractionEnd);
}

void AMyVehiclePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (StoredPawn && !GetController())
    {
        if (APlayerController* PC = Cast<APlayerController>(StoredPawn->GetController()))
        {
            if (PC->WasInputKeyJustPressed(EKeys::E))
            {
                EnterVehicle();
            }
        }
    }
}

void AMyVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMyVehiclePawn::DriveForward);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AMyVehiclePawn::Turn);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AMyVehiclePawn::ExitVehicle);
    PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AMyVehiclePawn::Fire);
}

void AMyVehiclePawn::OnInteractionBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        if (APawn* OtherPawn = Cast<APawn>(OtherActor))
        {
            StoredPawn = OtherPawn;
        }
    }
}

void AMyVehiclePawn::OnInteractionEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor == StoredPawn)
    {
        StoredPawn = nullptr;
    }
}

void AMyVehiclePawn::EnterVehicle()
{
    if (!StoredPawn) return;
    if (APlayerController* PC = Cast<APlayerController>(StoredPawn->GetController()))
    {
        PC->Possess(this);
    }
}

void AMyVehiclePawn::ExitVehicle()
{
    if (!StoredPawn) return;
    if (AController* C = GetController())
    {
        C->Possess(StoredPawn);
    }
}

void AMyVehiclePawn::Fire()
{
    OnVehicleFire.Broadcast();
}

void AMyVehiclePawn::DriveForward(float Value)
{
    if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        Movement->MaxSpeed = MoveSpeed;
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AMyVehiclePawn::Turn(float Value)
{
    if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
    {
        AddActorLocalRotation(FRotator(0.f, Value * TurnRate * GetWorld()->GetDeltaSeconds(), 0.f));
    }
}
