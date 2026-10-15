// Fill out your copyright notice in the Description page of Project Settings.


#include "TestPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

namespace TestPawnPickup
{
	static const FName InteractableTag(TEXT("InteractableActor"));
	static constexpr float RangeCm = 100.f;
}

// Sets default values
ATestPawn::ATestPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MoveSpeed = 100000.f;
	LookSensitivity = 1.f;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bFindCameraComponentWhenViewTarget = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Root);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ATestPawn::TestMoveForward(float Amount)
{
	ApplyLocalMove(Amount, 0.f, 0.f, 1.f);
}

void ATestPawn::BeginPlay()
{
	Super::BeginPlay();
	AutoPossessAI = EAutoPossessAI::Disabled;
	if (AController* Existing = GetController())
	{
		if (!Existing->IsA<APlayerController>())
		{
			Existing->UnPossess();
		}
	}
	AlignFollowCamera();
	ClaimPlayerView();
}

void ATestPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AlignFollowCamera();
	if (APlayerController* Player = Cast<APlayerController>(NewController))
	{
		UseThisAsViewTarget(Player);
	}
}

void ATestPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	AlignFollowCamera();
	ClaimPlayerView();
}

void ATestPawn::AlignFollowCamera()
{
	USceneComponent* Root = GetRootComponent();
	if (!CameraBoom || !FollowCamera || !Root)
	{
		return;
	}

	CameraBoom->AttachToComponent(Root, FAttachmentTransformRules::SnapToTargetIncludingScale);
	CameraBoom->SetAbsolute(false, false, false);
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetMobility(EComponentMobility::Movable);

	FollowCamera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetIncludingScale, USpringArmComponent::SocketName);
	FollowCamera->SetAbsolute(false, false, false);
	FollowCamera->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetMobility(EComponentMobility::Movable);
	FollowCamera->SetActive(true);
}

void ATestPawn::UseThisAsViewTarget(APlayerController* Player)
{
	if (!Player)
	{
		return;
	}

	Player->bAutoManageActiveCameraTarget = true;
	if (Player->GetViewTarget() != this)
	{
		Player->SetViewTarget(this);
	}
}

void ATestPawn::ClaimPlayerView()
{
	if (APlayerController* Existing = Cast<APlayerController>(GetController()))
	{
		UseThisAsViewTarget(Existing);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* Player = World->GetFirstPlayerController())
		{
			Player->Possess(this);
			UseThisAsViewTarget(Player);
		}
	}
}

void ATestPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (APlayerController* Player = Cast<APlayerController>(GetController()))
	{
		UseThisAsViewTarget(Player);
	}
	else
	{
		ClaimPlayerView();
	}

	const float Forward = FMath::Clamp(AxisForward + (AxisW - AxisS), -1.f, 1.f);
	const float Right = FMath::Clamp(AxisRight + (AxisD - AxisA), -1.f, 1.f);
	const float Up = AxisE - AxisQ;
	ApplyLocalMove(Forward, Right, Up, DeltaSeconds);
	UpdateJump(DeltaSeconds);
}

void ATestPawn::ApplyLocalMove(float Forward, float Right, float Up, float DeltaSeconds)
{
	if (FMath::IsNearlyZero(Forward) && FMath::IsNearlyZero(Right) && FMath::IsNearlyZero(Up))
	{
		return;
	}

	const FRotator YawOnly(0.f, GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawOnly);
	const FVector WorldDirection = (
		YawMatrix.GetUnitAxis(EAxis::X) * Forward
		+ YawMatrix.GetUnitAxis(EAxis::Y) * Right
		+ FVector::UpVector * Up).GetClampedToMaxSize(1.f);

	AddActorWorldOffset(WorldDirection * MoveSpeed * DeltaSeconds, false);
}

void ATestPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ATestPawn::TogglePickup);

		const FKey MoveKeys[] = { EKeys::W, EKeys::A, EKeys::S, EKeys::D, EKeys::Q, EKeys::E };
		for (const FKey& Key : MoveKeys)
		{
			PlayerInputComponent->BindKey(Key, IE_Pressed, this, &ATestPawn::OnMoveKeyPressed);
			PlayerInputComponent->BindKey(Key, IE_Released, this, &ATestPawn::OnMoveKeyReleased);
		}
		PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ATestPawn::Jump);
		PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATestPawn::OnMoveForwardAxis);
		PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATestPawn::OnMoveRightAxis);
		PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &ATestPawn::OnTurnAxis);
		PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &ATestPawn::OnLookUpAxis);
	}
}

void ATestPawn::OnMoveKeyPressed(FKey Key)
{
	SetMoveKey(Key, 1.f);
}

void ATestPawn::OnMoveKeyReleased(FKey Key)
{
	SetMoveKey(Key, 0.f);
}

void ATestPawn::Jump()
{
	// 已经腾空时忽略重复按键（空格按住不会连跳）
	if (bJumping)
	{
		return;
	}

	bJumping = true;
	JumpStartZ = GetActorLocation().Z;

	// 由 h = v^2 / (2g) 反解初速度，g 取 UE 默认重力 980 cm/s^2
	const float Gravity = 980.f;
	VerticalVelocity = FMath::Sqrt(2.f * Gravity * FMath::Max(JumpHeight, 0.f));
}

bool ATestPawn::IsJumping() const
{
	return bJumping;
}

void ATestPawn::UpdateJump(float DeltaSeconds)
{
	if (!bJumping)
	{
		return;
	}

	constexpr float Gravity = 980.f;
	VerticalVelocity -= Gravity * DeltaSeconds;

	FVector NewLocation = GetActorLocation();
	NewLocation.Z += VerticalVelocity * DeltaSeconds;

	// 回到或落到起跳高度以下时落地
	if (VerticalVelocity <= 0.f && NewLocation.Z <= JumpStartZ)
	{
		NewLocation.Z = JumpStartZ;
		VerticalVelocity = 0.f;
		bJumping = false;
	}

	AddActorWorldOffset(NewLocation - GetActorLocation(), false);
}

void ATestPawn::OnMoveForwardAxis(float Value)
{
	AxisForward = Value;
}

void ATestPawn::OnMoveRightAxis(float Value)
{
	AxisRight = Value;
}

void ATestPawn::OnTurnAxis(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddControllerYawInput(Value * LookSensitivity);
}

void ATestPawn::OnLookUpAxis(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddControllerPitchInput(-Value * LookSensitivity);
}

void ATestPawn::SetMoveKey(const FKey& Key, float Value)
{
	if (Key == EKeys::W) { AxisW = Value; }
	else if (Key == EKeys::S) { AxisS = Value; }
	else if (Key == EKeys::A) { AxisA = Value; }
	else if (Key == EKeys::D) { AxisD = Value; }
	else if (Key == EKeys::Q) { AxisQ = Value; }
	else if (Key == EKeys::E) { AxisE = Value; }
}

void ATestPawn::TogglePickup()
{
	if (IsValid(HeldActor))
	{
		HeldActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		HeldActor->SetActorTransform(HeldActorOrigin);
		HeldActor = nullptr;
		return;
	}

	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsWithTag(this, TestPawnPickup::InteractableTag, Candidates);

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(TestPawnPickup::RangeCm);
	const FVector Origin = GetActorLocation();
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate) || Candidate == this)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		return;
	}

	HeldActorOrigin = Best->GetActorTransform();
	Best->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	HeldActor = Best;
}


