#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "NewTestVehicle.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class ACharacter;

UCLASS(Blueprintable, BlueprintType)
class ANewTestVehicle : public APawn
{
    GENERATED_BODY()

public:
    ANewTestVehicle();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    USphereComponent* InteractSphere;

    UPROPERTY()
    ACharacter* StoredCharacter;

    UPROPERTY(EditAnywhere, Category="Vehicle")
    float MoveSpeed;

    UPROPERTY(EditAnywhere, Category="Vehicle")
    float TurnSpeed;

    void MoveForward(float Value);
    void Turn(float Value);
    void EnterVehicle();
    void ExitVehicle();
    bool IsPlayerNear() const;

    float ForwardInput;
    float TurnInput;
    bool bWasInteractDown;
};
