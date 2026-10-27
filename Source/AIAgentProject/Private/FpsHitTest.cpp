#include "FpsHealthComponent.h"
#include "FpsPawn.h"
#include "FpsTarget.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFpsHitTest,
	"AIAgent.Fps.Hit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFpsHitTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("没有引擎"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("FpsHitTest"));
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
	// 相机在胶囊中心上方 64uu，靶心对齐这条射线，避免从眼睛高度擦过盒顶。
	AFpsTarget* Target = World->SpawnActor<AFpsTarget>(FVector(400.f, 0.f, 64.f), FRotator::ZeroRotator, Spawn);
	bool Passed = TestNotNull(TEXT("Pawn"), Pawn) && TestNotNull(TEXT("Target"), Target);
	if (Pawn && Target && Target->Health)
	{
		Pawn->Fire();
		TestEqual(TEXT("命中数"), Pawn->HitCount, 1);
		TestEqual(TEXT("弹药"), Pawn->Ammo, 11);
		TestTrue(TEXT("靶耗尽"), Target->Health->IsDepleted());
		TestTrue(TEXT("靶隐藏"), Target->IsHidden());

		const int32 Hits = Pawn->HitCount;
		Pawn->Ammo = 0;
		Pawn->Fire();
		TestEqual(TEXT("没弹药不再命中"), Pawn->HitCount, Hits);
		TestEqual(TEXT("没弹药不再扣"), Pawn->Ammo, 0);

		const FVector Before = Pawn->GetActorLocation();
		Pawn->Drive(1.f, 0.f, 1.f);
		TestTrue(TEXT("向前移动"), Pawn->GetActorLocation().X > Before.X + 100.f);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return Passed;
}
