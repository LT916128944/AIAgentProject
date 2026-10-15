// Fill out your copyright notice in the Description page of Project Settings.


#include "TestGameMode.h"

#include "TestPawn.h"
#include "TestPlayerController.h"

ATestGameMode::ATestGameMode()
{
	DefaultPawnClass = ATestPawn::StaticClass();
	PlayerControllerClass = ATestPlayerController::StaticClass();
	MyDefaultPawn = ATestPawn::StaticClass();
	MyPlayerController = ATestPlayerController::StaticClass();
}
