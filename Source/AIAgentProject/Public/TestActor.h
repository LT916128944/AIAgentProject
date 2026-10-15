#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TestActor.generated.h"

/**
 * 测试 Actor，用于验证 UE 资产元数据导出与 RAG 检索。
 * 场景对象 实体 actor
 */
UCLASS()
class AIAGENTPROJECT_API ATestActor : public AActor
{
    GENERATED_BODY()

public:
    ATestActor();

    // 是否可被拾取（与 bPickable 同步，供蓝图/其他 Pawn 拾取逻辑查询）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Test")
    bool bCanBePicked;

    // 显示名 场景对象标签
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    FString DisplayName;

    // 血量 生命值 health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float Health;

    // 移动速度 speed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float MoveSpeed;

    // 缩放比例 scale
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    float Scale;

    // 是否可被拾取。勾选后自动拥有 InteractableActor 标签
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bPickable;

    // 是否允许被拾取（拾取总开关，关闭后任何 Pawn 都无法拾取）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
    bool bAllowPickup;

    // 设置是否允许被拾取，并同步 bCanBePicked 与 InteractableActor 标签
    UFUNCTION(BlueprintCallable, Category = "Test")
    void SetAllowPickup(bool bInAllowPickup);

    // 查询当前是否允许被拾取（读取 bAllowPickup 开关）
    UFUNCTION(BlueprintCallable, Category = "Test")
    bool IsAllowPickup() const;

    // 应用拾取标记：根据 bPickable 增删 InteractableActor 标签
    UFUNCTION(BlueprintCallable, Category = "Test")
    void ApplyPickable();

    // 查询当前是否可被拾取（依据 InteractableActor 标签判断）
    UFUNCTION(BlueprintCallable, Category = "Test")
    bool IsPickable() const;

    // 设置是否可被拾取，并同步 InteractableActor 标签
    UFUNCTION(BlueprintCallable, Category = "Test")
    void SetPickable(bool bInPickable);

    // 查询当前是否处于可拾取状态（读取 bCanBePicked 开关）
    UFUNCTION(BlueprintCallable, Category = "Test")
    bool CanBePicked() const;

    // 是否可被 Pawn 拾取：同时满足 bAllowPickup 与 bCanBePicked，且拥有 InteractableActor 标签
    UFUNCTION(BlueprintCallable, Category = "Test")
    bool CanBePickedByPawn() const;

    // 应用缩放比例到 Actor
    UFUNCTION(BlueprintCallable, Category = "Test")
    void ApplyScale();

    // 打印测试信息
    UFUNCTION(BlueprintCallable, Category = "Test")
    void TestPrintDebugInfo();
};