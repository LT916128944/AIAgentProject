#include "FpsGrenadePickup.h"
#include "FpsGrenadeProjectile.h"
#include "FpsExplosion.h"
#include "FpsHealthComponent.h"
#include "FpsPawn.h"
#include "FpsTarget.h"

#include "CollisionShape.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFpsGrenadeTest,
    "AIAgent.Fps.Grenade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	// 临时世界不自动 Tick（FFpsHitTest 教训）：投掷物与表现都由测试显式驱动
	bool DriveUntilExploded(UWorld* World, AFpsGrenadeProjectile* Grenade, float MaxSeconds)
	{
		float Driven = 0.f;
		while (IsValid(Grenade) && Driven < MaxSeconds)
		{
			Grenade->Tick(0.05f);
			Driven += 0.05f;
		}
		return !IsValid(Grenade);
	}
}

bool FFpsGrenadeTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("没有引擎"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("FpsGrenadeTest"));
	if (!TestNotNull(TEXT("临时世界"), World))
	{
		return false;
	}
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	FURL Url;
	World->InitializeActorsForPlay(Url);
	World->BeginPlay();

	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFpsPawn* Pawn = World->SpawnActor<AFpsPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	// 近靶留在爆点半径内，远靶在半径外：爆炸只该伤到近的
	AFpsTarget* NearTarget = World->SpawnActor<AFpsTarget>(FVector(400.f, 0.f, 64.f), FRotator::ZeroRotator, Spawn);
	AFpsTarget* FarTarget = World->SpawnActor<AFpsTarget>(FVector(3000.f, 0.f, 64.f), FRotator::ZeroRotator, Spawn);
	bool Passed = TestNotNull(TEXT("Pawn"), Pawn) && TestNotNull(TEXT("近靶"), NearTarget)
		&& TestNotNull(TEXT("远靶"), FarTarget);

	if (Passed && Pawn && NearTarget && FarTarget)
	{
		// 拾取：唯一入口 NotifyCollected，一次给 GrantCount 颗；重复通知不加
		AFpsGrenadePickup* Pickup = World->SpawnActor<AFpsGrenadePickup>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
		TestNotNull(TEXT("拾取物"), Pickup);
		if (Pickup)
		{
			Pickup->NotifyCollected(Pawn);
			TestEqual(TEXT("拾取加弹"), Pawn->GrenadeCount, Pickup->GrantCount);
			TestTrue(TEXT("拾取物标记已收"), Pickup->IsCollected());
			if (IsValid(Pickup))
			{
				const int32 Before = Pawn->GrenadeCount;
				Pickup->NotifyCollected(Pawn);
				TestEqual(TEXT("重复拾取不加"), Pawn->GrenadeCount, Before);
				Pickup->Destroy();
			}
		}

		// 投掷：有弹扣一、生成投掷物；没弹是 no-op，不生成也不扣
		Pawn->ThrowGrenade();
		const int32 Granted = Pickup ? Pickup->GrantCount : 1;
		TestEqual(TEXT("投掷扣弹"), Pawn->GrenadeCount, Granted - 1);
		int32 Flying = 0;
		for (TActorIterator<AFpsGrenadeProjectile> It(World); It; ++It)
		{
			Flying += 1;
		}
		TestEqual(TEXT("投掷生成投掷物"), Flying, 1);
		// 没弹段：显式清零再投，验证的是「没库存时不动」这条合同
		Pawn->GrenadeCount = 0;
		Pawn->ThrowGrenade();
		TestEqual(TEXT("没弹不扣"), Pawn->GrenadeCount, 0);
	}
	int32 FlyingAfterNoop = 0;
	for (TActorIterator<AFpsGrenadeProjectile> It(World); It; ++It)
	{
		FlyingAfterNoop += 1;
	}
	TestEqual(TEXT("没弹不生成"), FlyingAfterNoop, 1);

	// 投掷物会飞走，清掉，不让它干扰爆炸段的半径判定
	for (TActorIterator<AFpsGrenadeProjectile> It(World); It; ++It)
	{
		(*It)->Destroy();
	}
	FlushPersistentDebugLines(World);

	if (Passed && NearTarget && FarTarget)
	{
		// 爆炸段：投掷物直接生成在近靶位置，引信拨到 0，驱动一帧就炸
		FActorSpawnParameters Spawn2;
		Spawn2.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AFpsGrenadeProjectile* Grenade = World->SpawnActor<AFpsGrenadeProjectile>(
			FVector(400.f, 0.f, 64.f), FRotator::ZeroRotator, Spawn2);
		TestNotNull(TEXT("爆炸段投掷物"), Grenade);
		if (Grenade)
		{
			Grenade->FuseSeconds = 0.f;
			Grenade->Launch(Grenade->GetActorLocation(), FVector::ZeroVector);
			TestTrue(TEXT("引信到时自毁"), DriveUntilExploded(World, Grenade, 3.f));

			TestTrue(TEXT("近靶耗尽"), NearTarget->Health->IsDepleted());
			TestTrue(TEXT("近靶隐藏"), NearTarget->IsHidden());
			TestFalse(TEXT("远靶不掉血"), FarTarget->Health->IsDepleted());
			TestFalse(TEXT("远靶不隐藏"), FarTarget->IsHidden());

			// 表现：NS_Explosion 缺失时走 AFpsExplosion 兜底；两种至少要出一种
			AFpsExplosion* Fallback = nullptr;
			for (TActorIterator<AFpsExplosion> It(World); It; ++It)
			{
				Fallback = *It;
			}
			if (Fallback)
			{
				// 兜底表现也由测试驱动，寿命 0.6 秒到就自毁
				float Driven = 0.f;
				while (IsValid(Fallback) && Driven < 2.f)
				{
					Fallback->Tick(0.1f);
					Driven += 0.1f;
				}
				TestFalse(TEXT("兜底表现自毁"), IsValid(Fallback));
			}
			// NS 在场时 SpawnSystemAtLocation 的组件不归这条断言管；掉血与自毁才是合同
		}
	}

	if (Passed)
	{
		// 落地/命中即爆：脚下放一块方块，雷直坠，撞地要远早于引信（引信拨到 10 秒兜底）
		FActorSpawnParameters Spawn3;
		Spawn3.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(
			FVector(0.f, 0.f, -200.f), FRotator::ZeroRotator, Spawn3);
		if (UStaticMesh* Cube = Cast<UStaticMesh>(
			FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")).TryLoad()))
		{
			if (Floor && Floor->GetStaticMeshComponent())
			{
				Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
				Floor->GetStaticMeshComponent()->SetStaticMesh(Cube);
				Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
			}
		}
		TestNotNull(TEXT("撞地段地面"), Floor);
		if (Floor)
		{
			AFpsGrenadeProjectile* Dropper = World->SpawnActor<AFpsGrenadeProjectile>(
				FVector(0.f, 0.f, 64.f), FRotator::ZeroRotator, Spawn3);
			TestNotNull(TEXT("撞地段投掷物"), Dropper);
			if (Dropper)
			{
				Dropper->FuseSeconds = 10.f;
				Dropper->Launch(FVector(0.f, 0.f, 64.f), FVector(0.f, 0.f, -900.f));
				float Driven = 0.f;
				while (IsValid(Dropper) && Driven < 9.f)
				{
					Dropper->Tick(0.05f);
					Driven += 0.05f;
				}
				// 64 → -150（方块顶面）约 0.25 秒；引信要 10 秒。撞地先爆才算对
				TestTrue(TEXT("撞地先于引信爆炸"), Driven < 5.f);
				TestFalse(TEXT("撞地后自毁"), IsValid(Dropper));
			}
			Floor->Destroy();
		}
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return Passed;
}
