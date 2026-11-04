#include "FpsPawn.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "FpsBullet.h"
#include "FpsGrenadeProjectile.h"
#include "FpsHealthComponent.h"
#include "FpsTarget.h"
#include "SimpleVehicleTest.h"
#include "EngineUtils.h"
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

	// 血量真源：复用靶子那只 UFpsHealthComponent，口径 0..1（默认 1 = 满血）。
	// 本任务只做「有真值可读」，不挂扣血触发；HUD 侧负责乘成 0..100 显示。
	Health = CreateDefaultSubobject<UFpsHealthComponent>(TEXT("Health"));

	// FPS HUD：默认 /Game/UI/WBP_FpsHUD。软引用，不硬加载；蓝图/关卡实例可改。
	HudWidgetClass = TSoftClassPtr<UUserWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_FpsHUD.WBP_FpsHUD_C")));
}

float AFpsPawn::GetHealth01() const
{
	// 口径保持 0..1，不在这里乘 100：换算成显示值是 HUD 的事。
	// 组件没建出来（老蓝图实例/子类意外清空）时按满血返回，避免 HUD 读出 0 假死。
	if (!Health)
	{
		return 1.f;
	}
	return FMath::Clamp(Health->Health, 0.f, 1.f);
}

void AFpsPawn::BeginPlay()
{
	Super::BeginPlay();
	// 玩家控制器可能还没 Possess 到本 Pawn，EnsureHud 自己会在没控制器时早退；
	// PossessedBy / OnRep_Controller 之后由 Tick 再补一次（见 Tick 里的兜底）。
	EnsureHud();
}

void AFpsPawn::EnsureHud()
{
	if (HudInstance)
	{
		return;
	}
	UClass* WidgetClass = HudWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		// 资产还没做出来不算错：HUD 只是没显示，别刷屏。改完 WBP 重新 PIE 即可。
		// 记住「类不见了」，Tick 不再每帧重试加载。
		bHudClassMissing = true;
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !World->GetGameViewport())
	{
		// 没有视口就没有可挂的地方：Automation 的临时世界就是这种，别在里面建控件。
		return;
	}
	APlayerController* Player = Cast<APlayerController>(GetController());
	if (!Player)
	{
		Player = World->GetFirstPlayerController();
	}
	if (!Player)
	{
		// 还没被控制器占有：下次 Tick 再试
		return;
	}
	UUserWidget* Widget = CreateWidget<UUserWidget>(Player, WidgetClass);
	if (!Widget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] CreateWidget 失败：%s"), *WidgetClass->GetName());
		return;
	}
	Widget->AddToViewport();
	HudInstance = Widget;
	UE_LOG(LogTemp, Log, TEXT("[HUD] 已挂视口：%s"), *WidgetClass->GetName());
}

void AFpsPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Possess 之后才拿得到玩家控制器。BeginPlay 时早退过的，这里补挂一次（EnsureHud 幂等）。
	// 资产还没做出来时不重试，避免每帧 LoadSynchronous。
	if (!HudInstance && !bHudClassMissing)
	{
		EnsureHud();
	}
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
	// 注意：本类的成员 Health 是本 Pawn 自己的血量，这里打的是被打中那家伙的血量，
	// 所以局部变量必须换个名字，否则 C4458（声明隐藏类成员，本项目开成 error）。
	UFpsHealthComponent* TargetHealth = Hit.GetActor()->FindComponentByClass<UFpsHealthComponent>();
	if (!TargetHealth)
	{
		return;
	}
	TargetHealth->ApplyDamage(1.f);
	HitCount += 1;
	if (AFpsTarget* Target = Cast<AFpsTarget>(Hit.GetActor()))
	{
		Target->ReactToHit();
	}
}

void AFpsPawn::ThrowGrenade()
{
	// 没库存就不动：投掷不猜，扣数放在能真的抛出来之后
	if (GrenadeCount <= 0)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !ViewCamera)
	{
		return;
	}
	GrenadeCount -= 1;
	const FVector Start = ViewCamera->GetComponentLocation();
	const FVector Forward = ViewCamera->GetForwardVector();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	AFpsGrenadeProjectile* Grenade = World->SpawnActor<AFpsGrenadeProjectile>(Start, FRotator::ZeroRotator, Params);
	if (Grenade)
	{
		Grenade->Launch(Start, Forward * ThrowSpeed + FVector::UpVector * ThrowUpBias);
	}
}

void AFpsPawn::AddGrenades(int32 Count)
{
	if (Count > 0)
	{
		GrenadeCount += Count;
	}
}

void AFpsPawn::TryEnterVehicle()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	ASimpleVehiclePawn* Best = nullptr;
	float BestDistSq = 400.f * 400.f;
	for (TActorIterator<ASimpleVehiclePawn> It(World); It; ++It)
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
	if (!Best)
	{
		return;
	}
	AController* DriverController = GetController();
	if (!DriverController)
	{
		return;
	}
	// 交给载具：隐藏本 Pawn、Possess 载具；下车由载具的 E 负责
	Best->SetDriver(this);
	DriverController->Possess(Best);
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
	// 投掷键 G。BindKey 只有 4 参成员函数指针重载，不接受 lambda（已钉约定）
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AFpsPawn::ThrowGrenade);
	// 上车键 E。载具侧也有 E（下车），双方不同时持输入，不会双触发
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AFpsPawn::TryEnterVehicle);
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
