#include "NewTestVehicle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

ANewTestVehicle::ANewTestVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    MoveSpeed = 600.0f;
    TurnSpeed = 90.0f;
    ForwardInput = 0.0f;
    TurnInput = 0.0f;
    bWasInteractDown = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionProfileName(TEXT("Pawn"));

    InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
    InteractSphere->SetupAttachment(RootComponent);
    InteractSphere->InitSphereRadius(180.0f);
    InteractSphere->SetCollisionProfileName(TEXT("Trigger"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
        Mesh->SetRelativeScale3D(FVector(2.0f, 1.0f, 0.7f));
    }
}

void ANewTestVehicle::BeginPlay()
{
    Super::BeginPlay();
}

bool ANewTestVehicle::IsPlayerNear() const
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
    if (!PlayerPawn || PlayerPawn == this)
    {
        return false;
    }
    return FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation()) <= 250.0f;
}

void ANewTestVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        return;
    }

    const bool bInteractDown = PC->IsInputKeyDown(EKeys::E);
    if (bInteractDown && !bWasInteractDown)
    {
        if (PC->GetPawn() == this)
        {
            ExitVehicle();
        }
        else if (IsPlayerNear())
        {
            EnterVehicle();
        }
    }
    bWasInteractDown = bInteractDown;

    if (PC->GetPawn() == this)
    {
        if (!FMath::IsNearlyZero(ForwardInput))
        {
            const FVector Delta = GetActorForwardVector() * ForwardInput * MoveSpeed * DeltaSeconds;
            AddActorWorldOffset(Delta, true);
        }
        if (!FMath::IsNearlyZero(TurnInput))
        {
            FRotator NewRotation = GetActorRotation();
            NewRotation.Yaw += TurnInput * TurnSpeed * DeltaSeconds;
            SetActorRotation(NewRotation);
        }
    }
}

void ANewTestVehicle::MoveForward(float Value)
{
    ForwardInput = Value;
}

void ANewTestVehicle::Turn(float Value)
{
    TurnInput = Value;
}

void ANewTestVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (PlayerInputComponent)
    {
        PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ANewTestVehicle::MoveForward);
        PlayerInputComponent->BindAxis(TEXT("Turn"), this, &ANewTestVehicle::Turn);
    }
}

void ANewTestVehicle::EnterVehicle()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        return;
    }
    StoredCharacter = Cast<ACharacter>(PC->GetPawn());
    if (StoredCharacter)
    {
        PC->Possess(this);
    }
}

void ANewTestVehicle::ExitVehicle()
{
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC && StoredCharacter)
    {
        PC->Possess(StoredCharacter);
        StoredCharacter = nullptr;
    }
}
