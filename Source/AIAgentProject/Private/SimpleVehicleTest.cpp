#include "SimpleVehicleTest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "InputCoreTypes.h"
#include "CollisionQueryParams.h"

ASimpleVehiclePawn::ASimpleVehiclePawn()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMesh.Object);
    }
    BodyMesh->SetRelativeScale3D(FVector(2.0f, 1.0f, 0.5f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BodyMesh->SetCollisionProfileName(TEXT("Pawn"));
    // 车载自由视角相机：鼠标只转这个相机（控制旋转），车身朝向仍由 A/D 转向驱动
    VehicleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VehicleCamera"));
    VehicleCamera->SetupAttachment(SceneRoot);
    VehicleCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
    VehicleCamera->bUsePawnControlRotation = true;
    AutoPossessAI = EAutoPossessAI::Disabled;
    Throttle = 0.0f;
    Steer = 0.0f;
    bForwardPressed = false;
    bBackwardPressed = false;
    bLeftPressed = false;
    bRightPressed = false;
}

void ASimpleVehiclePawn::BeginPlay()
{
    Super::BeginPlay();
}

void ASimpleVehiclePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMovement(DeltaSeconds);
}

void ASimpleVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }
    PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ASimpleVehiclePawn::ForwardPressed);
    PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &ASimpleVehiclePawn::ForwardReleased);
    PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ASimpleVehiclePawn::BackwardPressed);
    PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &ASimpleVehiclePawn::BackwardReleased);
    PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ASimpleVehiclePawn::LeftPressed);
    PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &ASimpleVehiclePawn::LeftReleased);
    PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ASimpleVehiclePawn::RightPressed);
    PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &ASimpleVehiclePawn::RightReleased);
    PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASimpleVehiclePawn::InteractPressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASimpleVehiclePawn::FirePressed);
    // 车载视角：鼠标轴走控制旋转，相机 bUsePawnControlRotation 跟随
    PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &ASimpleVehiclePawn::TurnAxis);
    PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &ASimpleVehiclePawn::LookAxis);
}

void ASimpleVehiclePawn::TurnAxis(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddControllerYawInput(Value);
    }
}

void ASimpleVehiclePawn::LookAxis(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddControllerPitchInput(-Value);
    }
}

void ASimpleVehiclePawn::SetDriver(APawn* InDriver)
{
    Driver = InDriver;
    if (Driver)
    {
        Driver->SetActorHiddenInGame(true);
        Driver->SetActorEnableCollision(false);
    }
    bForwardPressed = false;
    bBackwardPressed = false;
    bLeftPressed = false;
    bRightPressed = false;
    Throttle = 0.0f;
    Steer = 0.0f;
}

void ASimpleVehiclePawn::ExitVehicle()
{
    if (!Driver)
    {
        return;
    }
    APawn* OldDriver = Driver;
    Driver = nullptr;
    LastSpeed = 0.0f;
    AController* C = GetController();
    if (C)
    {
        C->Possess(OldDriver);
    }
    const FVector ExitLocation = GetActorLocation() + GetActorRightVector() * -200.0f + FVector(0.0f, 0.0f, 100.0f);
    OldDriver->SetActorLocation(ExitLocation, false, nullptr, ETeleportType::TeleportPhysics);
    OldDriver->SetActorHiddenInGame(false);
    OldDriver->SetActorEnableCollision(true);
}

void ASimpleVehiclePawn::UpdateMovement(float DeltaSeconds)
{
    if (!Driver)
    {
        return;
    }
    const float NewThrottle = (bForwardPressed ? 1.0f : 0.0f) - (bBackwardPressed ? 1.0f : 0.0f);
    const float NewSteer = (bRightPressed ? 1.0f : 0.0f) - (bLeftPressed ? 1.0f : 0.0f);
    Throttle = FMath::FInterpTo(Throttle, NewThrottle, DeltaSeconds, 8.0f);
    Steer = FMath::FInterpTo(Steer, NewSteer, DeltaSeconds, 8.0f);
    const float Speed = 900.0f;
    const float TurnRate = 80.0f;
    AddActorLocalRotation(FRotator(0.0f, Steer * TurnRate * DeltaSeconds, 0.0f));
    AddActorLocalOffset(FVector(Throttle * Speed * DeltaSeconds, 0.0f, 0.0f), true);
    // 我们自己记速度：位移走 AddActorLocalOffset，没有 UMovementComponent，GetVelocity 恒为 0。
    // HUD 的「当前时速」读这个值。
    LastSpeed = FMath::Abs(Throttle * Speed);
}

FString ASimpleVehiclePawn::GetVehicleDisplayName() const
{
    // 实例上改过就是那个名字；没改过（或蓝图里留空）回落到关卡里看到的 Actor 标签。
    if (!VehicleDisplayName.IsEmpty())
    {
        return VehicleDisplayName;
    }
    return GetActorNameOrLabel();
}

void ASimpleVehiclePawn::ForwardPressed() { bForwardPressed = true; }
void ASimpleVehiclePawn::ForwardReleased() { bForwardPressed = false; }
void ASimpleVehiclePawn::BackwardPressed() { bBackwardPressed = true; }
void ASimpleVehiclePawn::BackwardReleased() { bBackwardPressed = false; }
void ASimpleVehiclePawn::LeftPressed() { bLeftPressed = true; }
void ASimpleVehiclePawn::LeftReleased() { bLeftPressed = false; }
void ASimpleVehiclePawn::RightPressed() { bRightPressed = true; }
void ASimpleVehiclePawn::RightReleased() { bRightPressed = false; }
void ASimpleVehiclePawn::InteractPressed() { ExitVehicle(); }
void ASimpleVehiclePawn::FirePressed() { Fire(); }

void ASimpleVehiclePawn::Fire()
{
    if (!Driver)
    {
        return;
    }
    // 车载开枪跟随视角：起点与方向都取自由视角相机
    const FVector Start = VehicleCamera
        ? VehicleCamera->GetComponentLocation()
        : GetActorLocation() + GetActorForwardVector() * 120.0f + FVector(0.0f, 0.0f, 50.0f);
    const FVector AimDirection = VehicleCamera ? VehicleCamera->GetForwardVector() : GetActorForwardVector();
    const FVector End = Start + AimDirection * 5000.0f;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Driver);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End, FColor::Red, false, 1.0f, 0, 2.0f);
    if (bHit)
    {
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20.0f, 12, FColor::Yellow, false, 1.0f);
        DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 12.0f, FColor::Green, false, 1.0f);
    }
}

ASimpleTestCharacter::ASimpleTestCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
    bForwardPressed = false;
    bBackwardPressed = false;
    bLeftPressed = false;
    bRightPressed = false;
}

void ASimpleTestCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ASimpleTestCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Forward = (bForwardPressed ? 1.0f : 0.0f) - (bBackwardPressed ? 1.0f : 0.0f);
    const float Right = (bRightPressed ? 1.0f : 0.0f) - (bLeftPressed ? 1.0f : 0.0f);
    if (Forward != 0.0f || Right != 0.0f)
    {
        const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
        const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        AddMovementInput(ForwardDir, Forward);
        AddMovementInput(RightDir, Right);
    }
}

void ASimpleTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!PlayerInputComponent)
    {
        return;
    }
    PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ASimpleTestCharacter::ForwardPressed);
    PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &ASimpleTestCharacter::ForwardReleased);
    PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ASimpleTestCharacter::BackwardPressed);
    PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &ASimpleTestCharacter::BackwardReleased);
    PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ASimpleTestCharacter::LeftPressed);
    PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &ASimpleTestCharacter::LeftReleased);
    PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ASimpleTestCharacter::RightPressed);
    PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &ASimpleTestCharacter::RightReleased);
    PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ASimpleTestCharacter::InteractPressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ASimpleTestCharacter::FirePressed);
}

void ASimpleTestCharacter::ForwardPressed() { bForwardPressed = true; }
void ASimpleTestCharacter::ForwardReleased() { bForwardPressed = false; }
void ASimpleTestCharacter::BackwardPressed() { bBackwardPressed = true; }
void ASimpleTestCharacter::BackwardReleased() { bBackwardPressed = false; }
void ASimpleTestCharacter::LeftPressed() { bLeftPressed = true; }
void ASimpleTestCharacter::LeftReleased() { bLeftPressed = false; }
void ASimpleTestCharacter::RightPressed() { bRightPressed = true; }
void ASimpleTestCharacter::RightReleased() { bRightPressed = false; }

void ASimpleTestCharacter::InteractPressed()
{
    ASimpleVehiclePawn* Vehicle = FindNearbyVehicle();
    if (Vehicle)
    {
        EnterVehicle(Vehicle);
    }
}

void ASimpleTestCharacter::FirePressed()
{
    Fire();
}

ASimpleVehiclePawn* ASimpleTestCharacter::FindNearbyVehicle() const
{
    ASimpleVehiclePawn* Best = nullptr;
    float BestDistSq = 400.0f * 400.0f;
    for (TActorIterator<ASimpleVehiclePawn> It(GetWorld()); It; ++It)
    {
        ASimpleVehiclePawn* Candidate = *It;
        if (!Candidate || Candidate->IsOccupied())
        {
            continue;
        }
        const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }
    return Best;
}

void ASimpleTestCharacter::EnterVehicle(ASimpleVehiclePawn* Vehicle)
{
    if (!Vehicle || Vehicle->IsOccupied())
    {
        return;
    }
    AController* C = GetController();
    if (!C)
    {
        return;
    }
    Vehicle->SetDriver(this);
    C->Possess(Vehicle);
}

void ASimpleTestCharacter::Fire()
{
    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
    const FVector End = Start + GetActorForwardVector() * 3000.0f;
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    DrawDebugLine(GetWorld(), Start, bHit ? Hit.ImpactPoint : End, FColor::Blue, false, 1.0f, 0, 2.0f);
    if (bHit)
    {
        DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 20.0f, 12, FColor::Cyan, false, 1.0f);
    }
}

// 默认 0：不再每个游戏世界自动生成载具试装并抢 Possess——那会把 FpsGameMode
// 生成的 FpsPawn 夺权，视角/射击/手雷全部失联（20261028 用户 PIE 实测）。
// 要试载具：控制台敲 simplevehicle.AutoSpawn 1 再进 PIE。
static TAutoConsoleVariable<bool> CVarSimpleVehicleAutoSpawn(
    TEXT("simplevehicle.AutoSpawn"), 0,
    TEXT("Auto-spawn the SimpleVehicle test rig and possess it in every game world. Default off."));

void USimpleVehicleWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld())
    {
        return;
    }
    if (!CVarSimpleVehicleAutoSpawn.GetValueOnGameThread())
    {
        return;
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnedVehicle = InWorld.SpawnActor<ASimpleVehiclePawn>(ASimpleVehiclePawn::StaticClass(), FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params);
    SpawnedCharacter = InWorld.SpawnActor<ASimpleTestCharacter>(ASimpleTestCharacter::StaticClass(), FVector(300.0f, 0.0f, 120.0f), FRotator::ZeroRotator, Params);
    APlayerController* PC = InWorld.GetFirstPlayerController();
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(&InWorld, 0);
    }
    if (PC && SpawnedCharacter)
    {
        PC->Possess(SpawnedCharacter);
    }
}
