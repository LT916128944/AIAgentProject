#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Subsystems/WorldSubsystem.h"
#include "SimpleVehicleTest.generated.h"

class UStaticMeshComponent;
class UInputComponent;
class USceneComponent;
class UCameraComponent;

UCLASS()
class ASimpleVehiclePawn : public APawn
{
    GENERATED_BODY()
public:
    ASimpleVehiclePawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void SetDriver(APawn* InDriver);
    void ExitVehicle();
    bool IsOccupied() const { return Driver != nullptr; }

    /** HUD 用：载具显示名。没设 VehicleDisplayName 时回落到 Actor 标签。 */
    UFUNCTION(BlueprintPure, Category = "Vehicle")
    FString GetVehicleDisplayName() const;

    /** HUD 用：当前车速（uu/s，恒为正）。位移是直接改位置，GetVelocity 恒为 0，所以自己记。 */
    UFUNCTION(BlueprintPure, Category = "Vehicle")
    float GetSpeedUuPerSec() const { return LastSpeed; }

    /** HUD/蓝图用：车上的人。没上车返回 nullptr。 */
    UFUNCTION(BlueprintPure, Category = "Vehicle")
    APawn* GetDriver() const { return Driver; }

    /** HUD 用：载具显示名（可在蓝图或关卡实例里改）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vehicle")
    FString VehicleDisplayName = TEXT("简易载具");

protected:
    virtual void BeginPlay() override;
private:
    /** 最近一帧的实际位移速度，UpdateMovement 写，HUD 读。 */
    float LastSpeed = 0.0f;
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* BodyMesh;
    // 车载自由视角：bUsePawnControlRotation 跟控制旋转，车身朝向不跟鼠标
    UPROPERTY(VisibleAnywhere)
    UCameraComponent* VehicleCamera;
    UPROPERTY()
    APawn* Driver;
    float Throttle;
    float Steer;
    bool bForwardPressed;
    bool bBackwardPressed;
    bool bLeftPressed;
    bool bRightPressed;
    void ForwardPressed();
    void ForwardReleased();
    void BackwardPressed();
    void BackwardReleased();
    void LeftPressed();
    void LeftReleased();
    void RightPressed();
    void RightReleased();
    void TurnAxis(float Value);
    void LookAxis(float Value);
    void InteractPressed();
    void FirePressed();
    void UpdateMovement(float DeltaSeconds);
    void Fire();
};

UCLASS()
class ASimpleTestCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ASimpleTestCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void EnterVehicle(ASimpleVehiclePawn* Vehicle);
protected:
    virtual void BeginPlay() override;
private:
    bool bForwardPressed;
    bool bBackwardPressed;
    bool bLeftPressed;
    bool bRightPressed;
    void ForwardPressed();
    void ForwardReleased();
    void BackwardPressed();
    void BackwardReleased();
    void LeftPressed();
    void LeftReleased();
    void RightPressed();
    void RightReleased();
    void InteractPressed();
    void FirePressed();
    ASimpleVehiclePawn* FindNearbyVehicle() const;
    void Fire();
};

UCLASS()
class USimpleVehicleWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
private:
    UPROPERTY()
    ASimpleTestCharacter* SpawnedCharacter;
    UPROPERTY()
    ASimpleVehiclePawn* SpawnedVehicle;
};
