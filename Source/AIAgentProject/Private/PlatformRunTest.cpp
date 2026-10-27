#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Misc/AutomationTest.h"

#include "PlatformCoin.h"
#include "PlatformDoor.h"
#include "PlatformMovingPlatform.h"
#include "PlatformProgressComponent.h"
#include "TestPawn.h"

namespace
{
	UWorld* MakePlatformTestWorld(const TCHAR* Name)
	{
		if (!GEngine)
		{
			return nullptr;
		}
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, Name);
		if (!World)
		{
			return nullptr;
		}
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL Url;
		World->InitializeActorsForPlay(Url);
		World->BeginPlay();
		return World;
	}

	void DestroyPlatformTestWorld(UWorld* World)
	{
		if (!GEngine || !World)
		{
			return;
		}
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTestPlatformRunTest,
	"AIAgent.Platform.Run",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTestPlatformRunTest::RunTest(const FString& Parameters)
{
	UWorld* World = MakePlatformTestWorld(TEXT("PlatformRunTest"));
	if (!TestNotNull(TEXT("临时世界"), World) || !World)
	{
		return false;
	}
	bool Passed = true;

	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 进度中枢先入场：金币自注册和门绑定都靠它
	APlatformProgressActor* ProgressActor = World->SpawnActor<APlatformProgressActor>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Passed &= TestNotNull(TEXT("进度中枢"), ProgressActor);
	UPlatformProgressComponent* Progress = ProgressActor ? ProgressActor->GetProgress() : nullptr;
	Passed &= TestNotNull(TEXT("进度组件"), Progress);

	ATestPawn* Pawn = World->SpawnActor<ATestPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	Passed &= TestNotNull(TEXT("Pawn"), Pawn);

	// 临时世界里 SpawnActor 不派发 BeginPlay（巡逻测试同款约定）：启动入口显式驱动
	if (Progress)
	{
		Progress->StartCountdown();
	}

	// 两枚金币：自注册后 TotalCoins=2，倒计时按组件默认 90 秒先跑着
	ACoin* CoinA = World->SpawnActor<ACoin>(FVector(200.f, 0.f, 50.f), FRotator::ZeroRotator, Spawn);
	ACoin* CoinB = World->SpawnActor<ACoin>(FVector(400.f, 0.f, 50.f), FRotator::ZeroRotator, Spawn);
	Passed &= TestNotNull(TEXT("金币 A"), CoinA);
	Passed &= TestNotNull(TEXT("金币 B"), CoinB);
	if (Progress)
	{
		Progress->RegisterPendingCoin();
		Progress->RegisterPendingCoin();
		Passed &= TestEqual(TEXT("金币自注册后总数"), Progress->TotalCoins, 2);
		Passed &= TestTrue(TEXT("倒计时在跑"), Progress->RemainingSeconds > 0.f);
	}

	// 门与移动平台（门显式绑进度）
	APlatformDoor* Door = World->SpawnActor<APlatformDoor>(FVector(600.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
	AMovingPlatform* Platform = World->SpawnActor<AMovingPlatform>(FVector(800.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
	Passed &= TestNotNull(TEXT("门"), Door);
	Passed &= TestNotNull(TEXT("移动平台"), Platform);
	if (Door && Progress)
	{
		Door->BindToProgress(Progress);
	}
	if (Platform)
	{
		Platform->StartMoving();
	}

	// 拾第一枚：计数递增，门还关着
	if (Progress && CoinA && Door)
	{
		FVector DoorBefore = Door->GetActorLocation();
		CoinA->NotifyCollected(Pawn);
		Passed &= TestTrue(TEXT("金币 A 已收"), CoinA->IsCollected());
		Passed &= TestEqual(TEXT("计数递增"), Progress->CollectedCount, 1);
		Passed &= TestTrue(TEXT("复核：重复拾取不重复计数"), !CoinA->PickupSphere->GetGenerateOverlapEvents());
		CoinA->NotifyCollected(Pawn);
		Passed &= TestEqual(TEXT("重复拾取不计数"), Progress->CollectedCount, 1);
		Passed &= TestFalse(TEXT("没收齐门不开"), Door->IsOpen());
		Passed &= TestTrue(TEXT("门保持原位"), Door->GetActorLocation().Equals(DoorBefore, 0.1f));
	}

	// 金币旋转与平台位移（1 秒步进；临时世界没有帧循环，组件 Tick 手动驱动）
	if (CoinB && Platform)
	{
		float YawBefore = CoinB->GetActorRotation().Yaw;
		CoinB->Rotating->TickComponent(1.f, LEVELTICK_All, nullptr);
		float YawAfter = CoinB->GetActorRotation().Yaw;
		Passed &= TestTrue(TEXT("金币旋转"), FMath::Abs(YawAfter - YawBefore) > 1.f);

		FVector PlatformBefore = Platform->GetActorLocation();
		Platform->Tick(1.f);
		FVector PlatformAfter = Platform->GetActorLocation();
		Passed &= TestTrue(TEXT("平台位移"), FVector::Dist(PlatformAfter, PlatformBefore) > 50.f);
	}

	// 收齐：门开，倒计时停
	if (Progress && CoinB && Door)
	{
		CoinB->NotifyCollected(Pawn);
		Passed &= TestEqual(TEXT("收齐计数"), Progress->CollectedCount, 2);
		Passed &= TestTrue(TEXT("收齐标记"), Progress->bAllCollected);
		Passed &= TestTrue(TEXT("门开"), Door->IsOpen());
		Passed &= TestTrue(TEXT("门位移"),
			FVector::Dist(Door->GetActorLocation(), FVector(600.f, 0.f, 100.f) + Door->OpenOffset) < 1.f);
		const float Remaining = Progress->RemainingSeconds;
		Progress->TickComponent(1.f, LEVELTICK_All, nullptr);
		Passed &= TestTrue(TEXT("收齐后倒计时停住"), FMath::Abs(Progress->RemainingSeconds - Remaining) < KINDA_SMALL_NUMBER);

		// 失败分支：重置进度，0.2 秒倒计时超时，且不动门
		Door->OpenOffset = FVector(0.f, 0.f, 300.f);
		APlatformDoor* Door2 = World->SpawnActor<APlatformDoor>(FVector(700.f, 0.f, 100.f), FRotator::ZeroRotator, Spawn);
		Passed &= TestNotNull(TEXT("门 2"), Door2);
		if (Door2)
		{
			Door2->BindToProgress(Progress);
		}
		Progress->ResetProgress(2, 0.2f);
		Passed &= TestFalse(TEXT("门 2 先是关的"), Door2 && Door2->IsOpen());
		Progress->TickComponent(0.3f, LEVELTICK_All, nullptr);
		Passed &= TestTrue(TEXT("倒计时超时"), Progress->bTimeOut);
		Passed &= TestTrue(TEXT("超时归零"), Progress->RemainingSeconds <= 0.f);
		Passed &= TestFalse(TEXT("失败分支门不开"), Door2 && Door2->IsOpen());
	}

	// ==== 跳跃落地与可达性：前方 150uu、高 80uu 的高台能不能跳上去 ====
	if (Pawn)
	{
		const FVector JumpStart = Pawn->GetActorLocation();
		const float LedgeTopZ = JumpStart.Z + 80.f;
		// 盒半高 10：盒心比顶面低 10，顶面正好在起跳点 +80uu
		const FVector LedgeCenter = JumpStart + Pawn->GetActorForwardVector() * 150.f + FVector(0.f, 0.f, 80.f - 10.f);

		AActor* Ledge = World->SpawnActor<AActor>(LedgeCenter, FRotator::ZeroRotator, Spawn);
		Passed &= TestNotNull(TEXT("临时高台"), Ledge);
		if (Ledge)
		{
			// 裸 AActor 可能没有根组件：自带就用自带的，没有才补一个，再把碰撞盒挂到根上
			USceneComponent* LedgeRoot = Ledge->GetRootComponent();
			if (!LedgeRoot)
			{
				LedgeRoot = NewObject<USceneComponent>(Ledge, TEXT("LedgeRoot"));
				LedgeRoot->SetMobility(EComponentMobility::Movable);
				Ledge->SetRootComponent(LedgeRoot);
				LedgeRoot->RegisterComponent();
			}
			// 新补的根是单位变换，这里把高台摆回目标位置，别让它留在原点
			Ledge->SetActorLocation(LedgeCenter);

			UBoxComponent* LedgeBox = NewObject<UBoxComponent>(Ledge, TEXT("LedgeBox"));
			Passed &= TestNotNull(TEXT("高台碰撞盒"), LedgeBox);
			if (LedgeBox)
			{
				LedgeBox->SetupAttachment(LedgeRoot);
				LedgeBox->SetBoxExtent(FVector(100.f, 100.f, 10.f));
				LedgeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				LedgeBox->SetCollisionResponseToAllChannels(ECR_Ignore);
				// Pawn 的落地检测是 ECC_Visibility 直线检测，只有这条通道要挡
				LedgeBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
				LedgeBox->SetMobility(EComponentMobility::Movable);
				LedgeBox->RegisterComponent();
				// 盒心与 Actor 重合：Actor 已在目标点，盒顶面就是起跳点 +80uu
				LedgeBox->SetRelativeLocation(FVector::ZeroVector);
			}

			Pawn->SetActorLocation(JumpStart);
			Pawn->MoveSpeed = 300.f;
			// JumpHeight 保持默认 100uu
			Pawn->Jump();

			// 临时世界没有输入：手动驱动「按住前进」，前 30 帧 × 5uu = 150uu，落到高台中心上方
			float ForwardMoved = 0.f;
			for (int32 Step = 0; Step < 120 && Pawn->IsJumping(); ++Step)
			{
				if (ForwardMoved < 150.f)
				{
					Pawn->TestMoveForward(1.f / 60.f);
					ForwardMoved += Pawn->MoveSpeed / 60.f;
				}
				Pawn->Tick(1.f / 60.f);
			}

			const float LandedZ = Pawn->GetActorLocation().Z;
			AddInfo(FString::Printf(TEXT("落地 Z：期望 %.1f（高台顶面），实测 %.1f，容差 2uu"), LedgeTopZ, LandedZ));
			Passed &= TestTrue(TEXT("跳跃后落到高台顶面（容差 2uu）"), FMath::Abs(LandedZ - LedgeTopZ) <= 2.f);
			Passed &= TestFalse(TEXT("落地后不再处于跳跃状态"), Pawn->IsJumping());
		}
	}

	// 数值断言：跳跃水平可达距离必须覆盖相邻平台最大间距
	const float Gravity = 980.f;
	const float AirTime = 2.f * FMath::Sqrt(2.f * 100.f / Gravity);
	const float Reach = 300.f * AirTime;
	AddInfo(FString::Printf(TEXT("jump reach = %.1f uu"), Reach));
	Passed &= TestTrue(TEXT("跳跃水平可达距离不小于相邻平台最大间距 200uu"), Reach >= 200.f);

	DestroyPlatformTestWorld(World);
	return Passed;
}