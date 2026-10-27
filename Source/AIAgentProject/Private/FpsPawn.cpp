#include "FpsPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "FpsBullet.h"
#include "FpsHealthComponent.h"
#include "FpsTarget.h"
#include "GameFramework/PlayerController.h"

AFpsPawn::AFpsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	Collision->InitCapsuleSize(42.f, 96.f);
	Collision->SetMobility(EComponentMobility::Movable);
	// 位移不靠阻挡。Block 会盖掉别的重叠，这里只查询。
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	SetRootComponent(Collision);

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(Collision);
	ViewCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	ViewCamera->bUsePawnControlRotation = true;
}

void AFpsPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Forward = FMath::Clamp(AxisForward + (AxisW - AxisS), -1.f, 1.f);
	const float Right = FMath::Clamp(AxisRight + (AxisD - AxisA), -1.f, 1.f);
	ApplyLocalMove(Forward, Right, DeltaSeconds);
}

void AFpsPawn::ApplyLocalMove(float Forward, float Right, float DeltaSeconds)
{
	if (FMath::IsNearlyZero(Forward) && FMath::IsNearlyZero(Right))
	{
		return;
	}
	float Yaw = GetActorRotation().Yaw;
	if (const APlayerController* Player = Cast<APlayerController>(GetController()))
	{
		Yaw = Player->GetControlRotation().Yaw;
	}
	const FRotationMatrix YawMatrix(FRotator(0.f, Yaw, 0.f));
	const FVector WorldDirection = (
		YawMatrix.GetUnitAxis(EAxis::X) * Forward
		+ YawMatrix.GetUnitAxis(EAxis::Y) * Right).GetClampedToMaxSize(1.f);
	AddActorWorldOffset(WorldDirection * MoveSpeed * DeltaSeconds, false);
}

void AFpsPawn::Drive(float Forward, float Right, float DeltaSeconds)
{
	ApplyLocalMove(Forward, Right, DeltaSeconds);
}

void AFpsPawn::Fire()
{
	if (Ammo <= 0)
	{
		return;
	}
	Ammo -= 1;

	UWorld* World = GetWorld();
	if (!World || !ViewCamera)
	{
		return;
	}
	const FVector Start = ViewCamera->GetComponentLocation();
	const FVector Forward = ViewCamera->GetForwardVector();
	const FVector End = Start + Forward * TraceRange;
	FCollisionQueryParams Params(TEXT("FpsFire"), false, this);
	Params.AddIgnoredActor(this);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetActor();
	const FVector Impact = (bHit && Hit.bBlockingHit) ? FVector(Hit.ImpactPoint) : (Start + Forward * 15000.f);
	const FVector Muzzle = Start + Forward * FMath::Min(60.f, FVector::Dist(Start, Impact));
	ShowBullet(Muzzle, Impact);
	if (!bHit)
	{
		return;
	}
	UFpsHealthComponent* Health = Hit.GetActor()->FindComponentByClass<UFpsHealthComponent>();
	if (!Health)
	{
		return;
	}
	Health->ApplyDamage(1.f);
	HitCount += 1;
	if (AFpsTarget* Target = Cast<AFpsTarget>(Hit.GetActor()))
	{
		Target->ReactToHit();
	}
}

void AFpsPawn::ShowBullet(const FVector& From, const FVector& To)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	AFpsBullet* Bullet = World->SpawnActor<AFpsBullet>(From, FRotator::ZeroRotator, Params);
	if (Bullet)
	{
		Bullet->Launch(From, To);
	}
}

void AFpsPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent)
	{
		return;
	}
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFpsPawn::Fire);
	const FKey MoveKeys[] = { EKeys::W, EKeys::A, EKeys::S, EKeys::D };
	for (const FKey& Key : MoveKeys)
	{
		PlayerInputComponent->BindKey(Key, IE_Pressed, this, &AFpsPawn::OnMoveKeyPressed);
		PlayerInputComponent->BindKey(Key, IE_Released, this, &AFpsPawn::OnMoveKeyReleased);
	}
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AFpsPawn::OnMoveForwardAxis);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AFpsPawn::OnMoveRightAxis);
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AFpsPawn::OnTurnAxis);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AFpsPawn::OnLookUpAxis);
}

void AFpsPawn::OnMoveKeyPressed(FKey Key)
{
	SetMoveKey(Key, 1.f);
}

void AFpsPawn::OnMoveKeyReleased(FKey Key)
{
	SetMoveKey(Key, 0.f);
}

void AFpsPawn::OnMoveForwardAxis(float Value)
{
	AxisForward = Value;
}

void AFpsPawn::OnMoveRightAxis(float Value)
{
	AxisRight = Value;
}

void AFpsPawn::OnTurnAxis(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerYawInput(Value * LookSensitivity);
	}
}

void AFpsPawn::OnLookUpAxis(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	AddControllerPitchInput(-Value * LookSensitivity);
}

void AFpsPawn::SetMoveKey(const FKey& Key, float Value)
{
	if (Key == EKeys::W) { AxisW = Value; }
	else if (Key == EKeys::S) { AxisS = Value; }
	else if (Key == EKeys::A) { AxisA = Value; }
	else if (Key == EKeys::D) { AxisD = Value; }
}
