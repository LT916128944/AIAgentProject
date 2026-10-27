// Fill out your copyright notice in the Description page of Project Settings.


#include "TestPawn.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PlatformHUDWidget.h"

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
	// 3 米/秒：按这个速度设计平台间距，压住空格能跳过去
	MoveSpeed = 300.f;
	LookSensitivity = 1.f;
	bCanBePossessed = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	bFindCameraComponentWhenViewTarget = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Root);

	// 碰撞胶囊：金币的球形重叠只认 Pawn，没有碰撞体就永远不触发拾取。
	// 不改成根，避免影响相机与已有的位移口径。
	// 位移走 AddActorWorldOffset(..., false)，不靠物理阻挡，所以这里只做查询、全部按重叠响应：
	// 只要一边是 Block，Block 会盖掉另一边的 Overlap，拾取就永远不触发（20261021 实测踩到）。
	Collision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Collision"));
	Collision->SetupAttachment(Root);
	Collision->InitCapsuleSize(34.f, 88.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_Pawn);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetRelativeLocation(FVector(0.f, 0.f, 88.f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(Root);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// 只存路径，不在这里加载资产：蓝图父类改完之前也能构造
	HudWidgetClass = TSoftClassPtr<UPlatformHUDWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));
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
	EnsureHud();
}

void ATestPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AlignFollowCamera();
	if (APlayerController* Player = Cast<APlayerController>(NewController))
	{
		UseThisAsViewTarget(Player);
	}
	EnsureHud();
}

void ATestPawn::PawnClientRestart()
{
	Super::PawnClientRestart();
	AlignFollowCamera();
	ClaimPlayerView();
	EnsureHud();
}

void ATestPawn::EnsureHud()
{
	if (HudInstance)
	{
		return;
	}
	UClass* WidgetClass = HudWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] HudWidgetClass 加载失败：%s"), *HudWidgetClass.ToString());
		return;
	}
	// HUD 挂到本地玩家控制器上。Pawn 上没有 PlayerOwner，只能从控制器或世界取
	APlayerController* Player = Cast<APlayerController>(GetController());
	if (!Player)
	{
		if (UWorld* World = GetWorld())
		{
			Player = World->GetFirstPlayerController();
		}
	}
	if (!Player)
	{
		// 还没被控制器占有：下一次 PossessedBy / PawnClientRestart 再来
		return;
	}
	// 没有视口就没有可挂的地方：Automation 的临时世界就是这种，别在里面建控件
	UWorld* World = GetWorld();
	if (!World || !World->GetGameViewport())
	{
		return;
	}
	HudInstance = CreateWidget<UPlatformHUDWidget>(Player, WidgetClass);
	if (!HudInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] CreateWidget 失败：%s"), *WidgetClass->GetName());
		return;
	}
	HudInstance->AddToViewport();
	UE_LOG(LogTemp, Log, TEXT("[HUD] 已挂视口：%s"), *WidgetClass->GetName());
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
	// 垂直更新每帧都跑：bJumping 只保留"空格还能不能再触发跳跃"，重力与贴地是常驻的
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
	// 常驻重力：不再由 bJumping 决定要不要跑垂直更新。
	// bJumping 只表示跳跃进行中（空格还能不能再触发跳跃），不表示"要不要受重力"。
	constexpr float Gravity = 980.f;
	VerticalVelocity -= Gravity * DeltaSeconds;

	const FVector Start = GetActorLocation();
	FVector NewLocation = Start;
	NewLocation.Z += VerticalVelocity * DeltaSeconds;

	// 支撑面统一用已有的 FindGroundBelow 向下探测，不另写一套探地。
	// 探测长度 = 本帧下落量 + GroundTraceExtra（5uu 容差），脚底贴面时也探得到。
	const float FallDistance = FMath::Max(Start.Z - NewLocation.Z, 0.f);
	const float TraceLength = FallDistance + GroundTraceExtra;

	float GroundZ = 0.f;
	// 只有在下落段或站住时做落地判定；上升段保持向上飞，不被地面夹住。
	if (VerticalVelocity <= 0.f && FindGroundBelow(TraceLength, GroundZ) && NewLocation.Z <= GroundZ)
	{
		// 脚底已落到或低于支撑面：夹到面上，垂直速度归零，跳跃结束。
		// 不往面里多埋：正好取支撑面高度。
		NewLocation.Z = GroundZ;
		VerticalVelocity = 0.f;
		bJumping = false;
	}
	// 探不到支撑面（走下平台）就什么都不夹，让重力继续把 Pawn 往下带。

	// 只改 Z：水平分量由 ApplyLocalMove 负责，这里不动，仍用 bSweep=false。
	AddActorWorldOffset(NewLocation - Start, false);
}

bool ATestPawn::FindGroundBelow(float TraceLength, float& OutGroundZ) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Feet = GetActorLocation();
	// 起点抬高一点，避免脚底正好贴在面上时起点已在碰撞体内被忽略
	const FVector TraceStart = Feet + FVector(0.f, 0.f, GroundTraceExtra);
	const FVector TraceEnd = FVector(Feet.X, Feet.Y, Feet.Z - FMath::Max(TraceLength, 0.f));

	FCollisionQueryParams Params(TEXT("TestPawnGroundTrace"), false, this);
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) || !Hit.bBlockingHit)
	{
		return false;
	}

	OutGroundZ = Hit.ImpactPoint.Z;
	return true;
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


