#include "TestPawn.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTestPawnWasdTest,
	"AIAgent.Pawn.Wasd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTestPawnWasdTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("没有引擎"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PawnWasdTest"));
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
	ATestPawn* Pawn = World->SpawnActor<ATestPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	bool Passed = TestNotNull(TEXT("Pawn"), Pawn);
	if (Pawn)
	{
		UInputComponent* Input = Pawn->InputComponent.Get();
		if (!Input)
		{
			Input = Pawn->CreatePlayerInputComponent();
			Pawn->InputComponent = Input;
			if (Input)
			{
				Input->RegisterComponent();
			}
		}
		Passed &= TestNotNull(TEXT("Input"), Input);
		if (Input)
		{
			Pawn->SetupPlayerInputComponent(Input);
			bool HasW = false;
			bool HasA = false;
			bool HasS = false;
			bool HasD = false;
			for (const FInputKeyBinding& Binding : Input->KeyBindings)
			{
				if (Binding.KeyEvent != IE_Pressed)
				{
					continue;
				}
				HasW |= Binding.Chord.Key == EKeys::W;
				HasA |= Binding.Chord.Key == EKeys::A;
				HasS |= Binding.Chord.Key == EKeys::S;
				HasD |= Binding.Chord.Key == EKeys::D;
			}
			Passed &= TestTrue(TEXT("绑定 W"), HasW);
			Passed &= TestTrue(TEXT("绑定 A"), HasA);
			Passed &= TestTrue(TEXT("绑定 S"), HasS);
			Passed &= TestTrue(TEXT("绑定 D"), HasD);

			bool HasForward = false;
			bool HasRight = false;
			for (const FInputAxisBinding& Binding : Input->AxisBindings)
			{
				HasForward |= Binding.AxisName == TEXT("MoveForward");
				HasRight |= Binding.AxisName == TEXT("MoveRight");
			}
			Passed &= TestTrue(TEXT("轴 MoveForward"), HasForward);
			Passed &= TestTrue(TEXT("轴 MoveRight"), HasRight);
		}

		const FVector Start = Pawn->GetActorLocation();
		Pawn->TestMoveForward(1.f);
		Passed &= TestTrue(TEXT("TestMoveForward 位移"), FVector::Dist(Pawn->GetActorLocation(), Start) > 50.f);

		const FVector Held = Pawn->GetActorLocation();
		Pawn->SetMoveKey(EKeys::D, 1.f);
		Pawn->Tick(1.f);
		Passed &= TestTrue(TEXT("按住 D 位移"), FVector::Dist(Pawn->GetActorLocation(), Held) > 50.f);
	}

	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return Passed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTestPawnLookTest,
	"AIAgent.Pawn.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTestPawnLookTest::RunTest(const FString& Parameters)
{
	if (!GEngine)
	{
		AddError(TEXT("没有引擎"));
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PawnLookTest"));
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
	ATestPawn* Pawn = World->SpawnActor<ATestPawn>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	bool Passed = TestNotNull(TEXT("Pawn"), Pawn);
	if (Pawn)
	{
		UInputComponent* Input = Pawn->InputComponent.Get();
		if (!Input)
		{
			Input = Pawn->CreatePlayerInputComponent();
			Pawn->InputComponent = Input;
			if (Input)
			{
				Input->RegisterComponent();
			}
		}
		Passed &= TestNotNull(TEXT("Input"), Input);
		if (Input)
		{
			Pawn->SetupPlayerInputComponent(Input);
			bool HasMouseX = false;
			bool HasMouseY = false;
			for (const FInputAxisKeyBinding& Binding : Input->AxisKeyBindings)
			{
				HasMouseX |= Binding.AxisKey == EKeys::MouseX;
				HasMouseY |= Binding.AxisKey == EKeys::MouseY;
			}
			Passed &= TestTrue(TEXT("绑定 MouseX"), HasMouseX);
			Passed &= TestTrue(TEXT("绑定 MouseY"), HasMouseY);
		}

		APlayerController* Player = World->SpawnActor<APlayerController>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
		Passed &= TestNotNull(TEXT("Player"), Player);
		if (Player)
		{
			Player->SetAsLocalPlayerController();
			Player->Possess(Pawn);
			const float Yaw0 = Player->GetControlRotation().Yaw;
			Pawn->OnTurnAxis(20.f);
			Player->UpdateRotation(0.016f);
			Passed &= TestTrue(TEXT("MouseX 改变 yaw"), FMath::Abs(Player->GetControlRotation().Yaw - Yaw0) > 1.f);

			const float Pitch0 = Player->GetControlRotation().Pitch;
			Pawn->OnLookUpAxis(20.f);
			Player->UpdateRotation(0.016f);
			Passed &= TestTrue(TEXT("MouseY 改变 pitch"), FMath::Abs(Player->GetControlRotation().Pitch - Pitch0) > 1.f);
		}
	}

	World->EndPlay(EEndPlayReason::Quit);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return Passed;
}
