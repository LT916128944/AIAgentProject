#include "Components/SphereComponent.h"
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

	DestroyPlatformTestWorld(World);
	return Passed;
}