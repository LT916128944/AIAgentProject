// Fill out your copyright notice in the Description page of Project Settings.


#include "TestPlayerController.h"

ATestPlayerController::ATestPlayerController()
{
}

void ATestPlayerController::HandleInput()
{
}

void ATestPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SetViewTarget(InPawn);
}
