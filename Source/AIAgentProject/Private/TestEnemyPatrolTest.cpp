#include "TestEnemy.h"
#include "TestPawn.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTestEnemyPatrolRuntimeTest,
	"AIAgent.Enemy.Patrol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTestEnemyPatrolRuntimeTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("没有引擎"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("EnemyPatrolTest"));
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
	ATestEnemy* Enemy = World->SpawnActor<ATestEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);

	bool Passed = TestNotNull(TEXT("敌人"), Enemy);
	float PatrolDistance = 0.f;
	float ChaseBefore = 0.f;
	float ChaseAfter = 0.f;
	int32 MeleeCount = 0;

	if (Enemy && Enemy->PatrolComponent)
	{
		auto Step = [Enemy](float DeltaTime)
		{
			Enemy->PatrolComponent->TickComponent(DeltaTime, LEVELTICK_All, nullptr);
		};

		const FVector Home = Enemy->GetActorLocation();
		for (int32 Index = 0; Index < 10; ++Index)
		{
			Step(0.1f);
		}
		PatrolDistance = FVector::Dist(Enemy->GetActorLocation(), Home);
		Passed &= TestTrue(*FString::Printf(TEXT("巡逻位移 %.1f"), PatrolDistance), PatrolDistance > 100.f);

		const FVector ChaseSpot = Enemy->GetActorLocation() + FVector(600.f, 0.f, 0.f);
		ATestPawn* Pawn = World->SpawnActor<ATestPawn>(ChaseSpot, FRotator::ZeroRotator, Spawn);
		APlayerController* Player = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
		Passed &= TestNotNull(TEXT("玩家"), Pawn);
		Passed &= TestNotNull(TEXT("控制器"), Player);
		if (Pawn && Player)
		{
			Player->SetAsLocalPlayerController();
			Player->Possess(Pawn);
			Pawn->SetActorLocation(ChaseSpot);
			Passed &= TestNotNull(TEXT("GetPlayerPawn"), UGameplayStatics::GetPlayerPawn(Enemy, 0));

			ChaseBefore = FVector::Dist(Enemy->GetActorLocation(), Pawn->GetActorLocation());
			for (int32 Index = 0; Index < 4; ++Index)
			{
				Step(0.1f);
			}
			ChaseAfter = FVector::Dist(Enemy->GetActorLocation(), Pawn->GetActorLocation());
			Passed &= TestTrue(
				*FString::Printf(TEXT("追击距离 %.1f -> %.1f"), ChaseBefore, ChaseAfter),
				ChaseAfter < ChaseBefore - 50.f);

			Pawn->SetActorLocation(Enemy->GetActorLocation() + FVector(80.f, 0.f, 0.f));
			Step(0.1f);
			MeleeCount = Enemy->PatrolComponent->MeleeAttackCount;
			Passed &= TestTrue(*FString::Printf(TEXT("近战次数 %d"), MeleeCount), MeleeCount >= 1);
		}
	}
	else if (Enemy)
	{
		AddError(TEXT("没有巡逻组件"));
		Passed = false;
	}

	AddWarning(FString::Printf(
		TEXT("reading patrol_uu=%.1f chase_before=%.1f chase_after=%.1f melee=%d"),
		PatrolDistance,
		ChaseBefore,
		ChaseAfter,
		MeleeCount));

	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return Passed;
}
