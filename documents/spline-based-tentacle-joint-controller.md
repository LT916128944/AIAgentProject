# 计划：基于 Spline 的关节式触手控制器（三状态切换）

## Summary

在 MeetVRTemplate 插件的 `Connector` 目录下新建一个独立的 Actor 类 `AMeetVRTentacleJointController`，实现基于关节点（初始点位）的触手 spline 控制。该类支持三个状态，每个状态拥有独立的关节点位数组，状态切换时关节点平滑插值过渡，spline 每帧根据插值后的关节点重建。

本实现与现有 `AMeetVRTentacleConnector`（正弦波动态 spline）完全独立、互不干扰。

---

## Current State Analysis（现状分析）

### 现有相关代码

1. **`AMeetVRTentacleConnector`**（[MeetVRTentacleConnector.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Connector/MeetVRTentacleConnector.h) / [.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp)）
   - 已存在的触手连接器，但基于**正弦波**动态生成 spline 点（Traveling/Standing 两种波形）。
   - 通过 `IMeetVRConnectorEndpoint` 接口连接 mother/tentacle 端点。
   - 每帧 `ClearSplinePoints` + `AddSplinePoint` 重建 spline。
   - 本次任务**不修改**此类。

2. **`ASuckerWeaponBase`**（[SuckerWeaponBase.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Sucker/SuckerWeaponBase.h) / [.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Sucker/SuckerWeaponBase.cpp)）
   - 提供状态机参考模式：`ESuckerWeaponState` 枚举（3 状态）+ `SetWeaponState()` + `OnWeaponStateChanged()` 虚函数回调 + `Disp_WeaponStateDispatcher` 委托广播。
   - 新类将复用此状态机模式。

3. **`MeetVRTemplate.Build.cs`**（[MeetVRTemplate.Build.cs](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/MeetVRTemplate.Build.cs)）
   - 已依赖 `Engine` 等模块，`USplineComponent` 无需额外依赖。**无需修改**。

### 约定与规范（来自用户偏好）
- 命名前缀 `meetvr` / `MeetVR`。
- 独立插件、无外部系统依赖。
- 注释使用中文（与用户沟通语言一致）。
- Blueprint 可调用，数据层完整。

---

## Proposed Changes（变更内容）

### 新建文件 1：`Public/Connector/MeetVRTentacleJointController.h`

完整路径：`f:\LiaoBo_VR\Plugins\MeetVRTemplate\Source\MeetVRTemplate\Public\Connector\MeetVRTentacleJointController.h`

内容要点：

```cpp
// Copyright 51Meet, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeetVRTentacleJointController.generated.h"

// 前向声明
class USceneComponent;
class USplineComponent;
class UCurveFloat;

// =============================================================================
// 触手关节状态枚举（三个状态）
// =============================================================================

UENUM(BlueprintType)
enum class EMeetVRTentacleJointState : uint8
{
    StateA    UMETA(DisplayName = "State A"),
    StateB    UMETA(DisplayName = "State B"),
    StateC    UMETA(DisplayName = "State C"),
};

// =============================================================================
// 状态变更委托
// =============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMeetVRTentacleJointStateChanged,
    uint8, NewState, UObject*, User);

// =============================================================================
// 关节式触手控制器
// =============================================================================

/**
 * AMeetVRTentacleJointController
 *
 * 基于关节点（初始点位）的触手 spline 控制器。
 * 三个状态各自持有一组关节点，状态切换时关节点平滑插值过渡，
 * spline 每帧根据插值后的关节点重建。
 *
 * 关节点在本地空间定义，整体跟随 Actor 变换移动。
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup = (MeetVR))
class MEETVRTEMPLATE_API AMeetVRTentacleJointController : public AActor
{
    GENERATED_BODY()

public:
    AMeetVRTentacleJointController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    //~AActor interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldTickIfViewportsOnly() const override;
    //~End of AActor interface

    // -------------------------------------------------------------------------
    // 状态机
    // -------------------------------------------------------------------------

    /** 设置目标状态，触发平滑过渡。bInstant=true 时立即跳变。 */
    UFUNCTION(BlueprintCallable, Category = "MeetVR|TentacleJoint")
    void SetTentacleState(EMeetVRTentacleJointState NewState, bool bInstant = false, UObject* User = nullptr);

    /** 获取当前（已抵达的）状态。 */
    UFUNCTION(BlueprintPure, Category = "MeetVR|TentacleJoint")
    EMeetVRTentacleJointState GetCurrentState() const { return CurrentState; }

    /** 获取正在过渡到的目标状态（过渡中与 CurrentState 不同）。 */
    UFUNCTION(BlueprintPure, Category = "MeetVR|TentacleJoint")
    EMeetVRTentacleJointState GetTargetState() const { return TargetState; }

    /** 是否正在过渡中。 */
    UFUNCTION(BlueprintPure, Category = "MeetVR|TentacleJoint")
    bool IsTransitioning() const { return bIsTransitioning; }

    /** 当前过渡进度 0~1。 */
    UFUNCTION(BlueprintPure, Category = "MeetVR|TentacleJoint")
    float GetTransitionAlpha() const { return TransitionAlpha; }

    // -------------------------------------------------------------------------
    // 状态变更事件
    // -------------------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "MeetVR|TentacleJoint|Events")
    FMeetVRTentacleJointStateChanged Disp_TentacleStateChanged;

    // -------------------------------------------------------------------------
    // 组件
    // -------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> DefaultSceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USplineComponent> TentacleSpline;

    // -------------------------------------------------------------------------
    // 各状态关节点（本地空间）
    // -------------------------------------------------------------------------

    /** State A 的关节点数组（本地空间）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joints|StateA")
    TArray<FVector> StateA_Joints;

    /** State B 的关节点数组（本地空间）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joints|StateB")
    TArray<FVector> StateB_Joints;

    /** State C 的关节点数组（本地空间）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Joints|StateC")
    TArray<FVector> StateC_Joints;

    // -------------------------------------------------------------------------
    // 过渡参数
    // -------------------------------------------------------------------------

    /** 过渡持续时间（秒）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition",
        meta = (ClampMin = "0.0"))
    float TransitionDuration = 0.5f;

    /** 可选过渡曲线（0~1 输入，0~1 输出）。为空时使用线性。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
    TObjectPtr<UCurveFloat> TransitionCurve;

    /** Spline 点类型（控制经过关节点时的曲线弯曲方式）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline")
    ESplinePointType::Type SplinePointType = ESplinePointType::Curve;

    /** 是否在编辑器视口绘制调试。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bDrawDebug = true;

protected:
    // -------------------------------------------------------------------------
    // 内部回调
    // -------------------------------------------------------------------------

    /** 状态变更内部处理（子类可重写）。 */
    virtual void OnTentacleStateChanged(EMeetVRTentacleJointState NewState, UObject* User);

    // -------------------------------------------------------------------------
    // 核心方法
    // -------------------------------------------------------------------------

    /** 取得指定状态的关节点数组（只读引用）。 */
    const TArray<FVector>& GetJointsForState(EMeetVRTentacleJointState State) const;

    /** 根据当前过渡进度插值出关节点，并重建 spline。 */
    void RebuildSplineFromJoints();

private:
    // 当前已抵达状态
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
    EMeetVRTentacleJointState CurrentState = EMeetVRTentacleJointState::StateA;

    // 过渡起始状态（过渡中用于 lerp 起点）
    EMeetVRTentacleJointState PreviousState = EMeetVRTentacleJointState::StateA;

    // 目标状态
    EMeetVRTentacleJointState TargetState = EMeetVRTentacleJointState::StateA;

    // 是否正在过渡
    bool bIsTransitioning = false;

    // 过渡进度 0~1
    float TransitionAlpha = 1.0f;

    // 过渡累计时间
    float TransitionElapsedTime = 0.0f;

    // 当前帧插值后的关节点缓存（本地空间）
    TArray<FVector> InterpolatedJoints;
};
```

### 新建文件 2：`Private/Connector/MeetVRTentacleJointController.cpp`

完整路径：`f:\LiaoBo_VR\Plugins\MeetVRTemplate\Source\MeetVRTemplate\Private\Connector\MeetVRTentacleJointController.cpp`

实现要点：

1. **构造函数**
   - 创建 `DefaultSceneRoot`（根）。
   - 创建 `TentacleSpline`，Attach 到根，开启 `SetDrawDebug` / `bDrawDebug`。
   - `PrimaryActorTick.bCanEverTick = true`，`bStartWithTickEnabled = true`。

2. **BeginPlay**
   - 初始化 `CurrentState = TargetState = StateA`（默认）。
   - `TransitionAlpha = 1.0f`，`bIsTransitioning = false`。
   - 立即 `RebuildSplineFromJoints()` 让初始 spline 与 StateA 一致。
   - 广播初始状态 `Disp_TentacleStateChanged`。

3. **SetTentacleState(NewState, bInstant, User)**
   - 若 `NewState == TargetState`：仅刷新广播（与 `ASuckerWeaponBase::SetWeaponState` 行为一致），return。
   - 记录 `PreviousState = CurrentState`，`TargetState = NewState`。
   - 若 `bInstant || TransitionDuration <= 0`：直接 `CurrentState = NewState`，`TransitionAlpha = 1`，`bIsTransitioning = false`，重建 spline，广播。
   - 否则：`bIsTransitioning = true`，`TransitionAlpha = 0`，`TransitionElapsedTime = 0`。
   - 调用 `OnTentacleStateChanged(NewState, User)`。

4. **Tick(DeltaTime)**
   - 若 `bIsTransitioning`：
     - `TransitionElapsedTime += DeltaTime`。
     - `TransitionAlpha = Clamp(TransitionElapsedTime / TransitionDuration, 0, 1)`。
     - 若 `TransitionCurve` 有效：`TransitionAlpha = TransitionCurve->GetFloatValue(TransitionAlpha)`。
     - 若 `TransitionAlpha >= 1`：过渡完成，`CurrentState = TargetState`，`bIsTransitioning = false`。
   - `RebuildSplineFromJoints()`。

5. **RebuildSplineFromJoints**
   - 取 `PrevJoints = GetJointsForState(PreviousState)`，`TargetJoints = GetJointsForState(TargetState)`。
   - 若过渡完成（非过渡中）：直接用 `TargetJoints`。
   - 计算插值点数 = `Min(PrevJoints.Num(), TargetJoints.Num())`；若两边都为空，return。
   - 若数量不一致：`UE_LOG(LogTemp, Warning, ...)` 提示，按最小数量插值。
   - 对每个 i：`InterpolatedJoints[i] = FMath::Lerp(PrevJoints[i], TargetJoints[i], TransitionAlpha)`（用 `PreviousState` 与 `TargetState` 的点做 lerp；过渡中 PreviousState 是上一稳定态、TargetState 是目标，alpha 从 0→1）。
   - `TentacleSpline->ClearSplinePoints()`。
   - 逐点 `AddSplinePoint(InterpolatedJoints[i], ESplineCoordinateSpace::Local, false)`，并 `SetSplinePointType(i, SplinePointType, false)`。
   - `TentacleSpline->UpdateSpline()`。

6. **GetJointsForState**
   - switch 返回 `StateA_Joints` / `StateB_Joints` / `StateC_Joints` 引用。

7. **OnTentacleStateChanged**
   - 默认空实现（virtual，供子类/蓝图重写），最后 `Disp_TentacleStateChanged.Broadcast((uint8)NewState, User)`。

8. **ShouldTickIfViewportsOnly**
   - 返回 `true`（与现有 Connector 一致，便于编辑器预览）。

---

## Assumptions & Decisions（假设与决策）

1. **独立类**：新建 `AMeetVRTentacleJointController`，不修改现有 `AMeetVRTentacleConnector`，二者并存。
2. **状态命名**：用户未指定三个状态语义，采用通用 `StateA/B/C`（DisplayName "State A/B/C"）。用户可在蓝图子类中按业务重命名或赋予语义。
3. **关节点空间**：关节点在**本地空间**定义，spline 点以 `ESplineCoordinateSpace::Local` 添加，整体随 Actor 变换移动。这样可通过移动/旋转 Actor 整体摆放触手。
4. **关节点数量约束**：三个状态的关节数组**应数量一致**以保证插值正确。数量不一致时打印 Warning 并按最小数量插值（不崩溃）。
5. **过渡方式**：逐关节线性 `FMath::Lerp`（位置插值），配合可选 `UCurveFloat` 控制整体 alpha 缓动（EaseInOut 等）。不做切线/旋转插值，保持简单。
6. **Spline 点类型**：默认 `ESplinePointType::Curve`（平滑经过关节），可整体配置。
7. **状态机模式**：复用 `ASuckerWeaponBase` 的 `SetState + virtual OnChanged + 委托广播` 模式，保持代码风格一致。
8. **过渡中再次切换状态**：以当前已插值到的 `PreviousState`→`TargetState` 重新开始过渡。简化处理：`PreviousState = CurrentState`（即过渡未完成时，PreviousState 仍是过渡起始的稳定态），从 alpha=0 重新过渡到新 TargetState。注：此为简化，不做"从当前插值位置续接"，因为 CurrentState 在过渡完成前不更新。
9. **无网络复制**：暂不实现 replication（与现有 Connector 一致，均为本地驱动）。
10. **Build.cs**：无需修改（`USplineComponent` 已由 `Engine` 模块提供）。

---

## Verification Steps（验证步骤）

1. **编译验证**：构建 MeetVRTemplate 模块，确认无编译错误。
2. **编辑器放置**：在关卡中拖入 `AMeetVRTentacleJointController`。
3. **配置关节点**：
   - 为 `StateA_Joints` / `StateB_Joints` / `StateC_Joints` 各填入相同数量的本地坐标点（如 5 个点，呈不同形状）。
   - 确认编辑器视口中 spline 显示为 StateA 形状。
4. **运行时切换**：
   - 蓝图或控制台调用 `SetTentacleState(StateB)` → 观察 spline 在 `TransitionDuration` 内平滑变形到 StateB。
   - 再调用 `SetTentacleState(StateC, bInstant=true)` → 观察 spline 立即跳变到 StateC。
5. **边界情况**：
   - 三个状态关节数量不一致 → 确认有 Warning 日志且不崩溃。
   - `TransitionDuration = 0` → 确认等同于瞬时切换。
   - `TransitionCurve` 指定一条 EaseInOut 曲线 → 确认过渡有缓动。
6. **事件广播**：绑定 `Disp_TentacleStateChanged`，确认切换时收到 (NewState, User)。
7. **编辑器预览**：`ShouldTickIfViewportsOnly=true`，确认在未运行时编辑器中修改关节点数组后 spline 实时更新（Tick 在视口也执行）。
