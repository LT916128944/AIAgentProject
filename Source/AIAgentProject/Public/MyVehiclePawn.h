#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MyVehiclePawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UFloatingPawnMovement;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVehicleFire);

UCLASS()
class AMyVehiclePawn : public APawn
{
    GENERATED_BODY()

public:
    AMyVehiclePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void EnterVehicle();

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void ExitVehicle();

    UFUNCTION(BlueprintCallable, Category="Vehicle")
    void Fire();

    UPROPERTY(BlueprintAssignable, Category="Vehicle")
    FOnVehicleFire OnVehicleFire;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnInteractionBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnInteractionEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void DriveForward(float Value);
    void Turn(float Value);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<UFloatingPawnMovement> Movement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<USphereComponent> InteractionSphere;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    TObjectPtr<APawn> StoredPawn;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    float MoveSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    float TurnRate = 90.f;
};
