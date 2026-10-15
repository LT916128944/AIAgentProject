# Connector Niagara 表现层实现计划（Ribbon + Array DI）

## Summary

为 `AMeetVRTentacleConnector` 添加 Niagara 表现层：用一个 `UNiagaraComponent` 配合 `NiagaraDataInterfaceArrayFloat3`（位置数组）驱动 Ribbon Renderer，每帧把 `ConnectorSpline` 的所有点世界坐标传给 Niagara，渲染一条连续平滑的"能量光带"触手。

C++ 侧只负责"把 Spline 点喂给 Niagara"；触手的发光、粗细、扰动等视觉表现全部在 Niagara System 资产里配置。功能逻辑（正弦波、行波/驻波、伸缩）完全不变。

## Current State Analysis

### 已有基础（无需改动）
- [MeetVRTemplate.Build.cs:29](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/MeetVRTemplate.Build.cs#L29) 已依赖 `Niagara` 模块 ✓
- [MeetVRTentacleConnector.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp) 的 `UpdateSineSpline` 每帧已计算出 `ConnectorSpline` 的所有点（世界坐标），可直接复用
- [SuckerParabolicTrajectoryVisualizerActor](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Actor/SuckerParabolicTrajectoryVisualizerActor.cpp) 提供了 `UNiagaraComponent` + `SetVariableVec3` 的项目惯用法参考

### 当前缺口
- `AMeetVRTentacleConnector` 没有 `UNiagaraComponent` 子组件
- 没有把 Spline 点传出去的接口
- 没有 Niagara System 资产（需用户在编辑器内创建）

## Proposed Changes

### 改动 1：头文件新增 Niagara 组件与资产引用 — [MeetVRTentacleConnector.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Connector/MeetVRTentacleConnector.h)

**新增 include**（顶部）：
```cpp
#include "NiagaraComponent.h"  // 放在 generated.h 之后
```
（或前向声明 `UNiagaraComponent` / `UNiagaraSystem`，在 .cpp 里 include。推荐前向声明 + cpp include，保持头文件轻量。）

**Components 区域新增**（`GuideSpline` 之后）：
```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
TObjectPtr<UNiagaraComponent> ConnectorVFX;
```

**Sine Wave Parameters 区域之后，新增 Visualization 区域**：
```cpp
// -------------------------------------------------------------------------
// Visualization (Niagara)
// -------------------------------------------------------------------------

/** Niagara System asset used to render the connector. Should use a Ribbon
 *  Renderer driven by a NiagaraDataInterfaceArrayFloat3 named "SplinePoints". */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
TObjectPtr<UNiagaraSystem> ConnectorSystem;

/** Name of the NiagaraDataInterfaceArrayFloat3 that receives spline points.
 *  Must match the DI name inside ConnectorSystem. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
FName SplinePointsArrayName = TEXT("SplinePoints");

/** If true, activates the Niagara component in BeginPlay. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visualization")
bool bAutoActivateVFX = true;
```

**Core Methods 区域新增私有方法**：
```cpp
/** Pushes current ConnectorSpline points into the Niagara array DI. */
void UpdateNiagaraSplinePoints();
```

### 改动 2：cpp 构造函数创建 Niagara 组件 — [MeetVRTentacleConnector.cpp:14-32](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L14-L32)

**新增 include**（文件顶部）：
```cpp
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFloat3.h"
```

**构造函数末尾新增**（`GuideSpline` 创建之后）：
```cpp
ConnectorVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ConnectorVFX"));
ConnectorVFX->SetupAttachment(DefaultSceneRoot);
ConnectorVFX->bAutoActivate = false;  // 等到有 System 再激活
```

### 改动 3：BeginPlay 激活 Niagara — [MeetVRTentacleConnector.cpp:38-48](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L38-L48)

在 `Super::BeginPlay()` 之后、现有逻辑之前新增：
```cpp
// Activate Niagara if a System asset is assigned
if (ConnectorVFX && ConnectorSystem)
{
    ConnectorVFX->SetAsset(ConnectorSystem);
    if (bAutoActivateVFX)
    {
        ConnectorVFX->Activate();
    }
}
```

### 改动 4：Tick 中调用更新 — [MeetVRTentacleConnector.cpp:102-106](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L102-L106)

在 `UpdateSineSpline` 之后、`DriveTentacleTo` 之前（或之后均可，放最后也行）新增：
```cpp
// 4b. Push spline points to Niagara for rendering
UpdateNiagaraSplinePoints();
```

### 改动 5：实现 UpdateNiagaraSplinePoints — [MeetVRTentacleConnector.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp)（文件末尾新增）

```cpp
void AMeetVRTentacleConnector::UpdateNiagaraSplinePoints()
{
    if (!ConnectorVFX || !ConnectorSpline)
    {
        return;
    }

    // Only update if the system uses the expected Array DI
    UNiagaraDataInterfaceArrayFloat3* ArrayDI =
        ConnectorVFX->Get Niagara... // 见下方说明
}
```

**实现要点**：
1. 从 `ConnectorSpline->GetNumberOfSplinePoints()` 获取点数
2. 遍历每个点，用 `ConnectorSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World)` 收集到 `TArray<FVector>`
3. 通过 `ConnectorVFX->SetVariableVec3Array(SplinePointsArrayName, PointsArray)` 一次性传给 Niagara

**关键 API**：UE5.6 中 `UNiagaraComponent` 直接支持 `SetVariableVec3Array(FName VariableName, const TArray<FVector>& Value)`，它会自动绑定到 Niagara System 内同名 `NiagaraDataInterfaceArrayFloat3`。

完整实现：
```cpp
void AMeetVRTentacleConnector::UpdateNiagaraSplinePoints()
{
    if (!ConnectorVFX || !ConnectorSpline)
    {
        return;
    }

    const int32 NumPoints = ConnectorSpline->GetNumberOfSplinePoints();
    if (NumPoints < 2)
    {
        return;
    }

    TArray<FVector> Points;
    Points.Reserve(NumPoints);
    for (int32 i = 0; i < NumPoints; ++i)
    {
        Points.Add(ConnectorSpline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
    }

    ConnectorVFX->SetVariableVec3Array(SplinePointsArrayName, Points);
}
```

## Niagara System 资产配置指南（用户在编辑器内完成）

C++ 改动完成后，需要用户在 Niagara 编辑器里创建一个 `NS_ConnectorRibbon` 资产，步骤如下：

### 步骤 1：创建 Niagara System
- Content Browser 右键 → Niagara → Niagara System → Empty
- 命名 `NS_ConnectorRibbon`，放在 `Plugins/MeetVRTemplate/Content/` 下

### 步骤 2：添加 Emitter
- Emitter 标签 → Add → 搜索 "Ribbon" → 选择一个 Ribbon 模板
- 或：Emitter → Update → Spawn Burst Instantaneous = 1（只生成一个持续粒子）

### 步骤 3：添加数据接口
- Emitter → +Parameter → Add → Data Interface → Array Float3
- 重命名为 `SplinePoints`（必须与 C++ 的 `SplinePointsArrayName` 一致）

### 步骤 4：用 Array DI 驱动粒子位置
- Particle Spawn → Position → 设为从 `SplinePoints` 数组采样
- 或用 Script 逐点 spawn 粒子（每点一个粒子），粒子按索引排列

### 步骤 5：配置 Ribbon Renderer
- Renderer → Add → Ribbon Renderer
- Binding: Position → Particle Position
- 设置 ribbon 宽度（可在材质里做沿长度的粗细变化）

### 步骤 6：材质
- 创建一个半透明发光材质 `M_ConnectorRibbon`：
  - Shading Model: Unlit
  - Blend Mode: Translucent
  - Emissive Color 接一个沿 ribbon UV 流动的纹理或噪声
  - 可加 Fresnel 让边缘更亮

### 步骤 7：在 Connector Actor 上指定资产
- 选中场景中的 `MeetVRTentacleConnector`
- Details 面板 → Visualization → Connector System = `NS_ConnectorRibbon`
- Spline Points Array Name = `SplinePoints`（默认值已匹配）

## Assumptions & Decisions

1. **Array DI 名称固定为 "SplinePoints"**：C++ 默认值与 Niagara 资产内名称必须一致。提供 `SplinePointsArrayName` 属性是为了让用户在改名时无需改代码。
2. **不依赖具体的 Niagara System**：C++ 只负责传数组，System 资产由用户配置。若 `ConnectorSystem` 为空，Niagara 组件不激活，ConnectorSpline 的 debug 线仍可看到运动。
3. **每帧传整个数组**：`SetVariableVec3Array` 每帧调用一次，点数固定（SplinePointCount），开销可控。
4. **不改变现有功能**：正弦波、行波/驻波、伸缩、GuideSpline 等逻辑完全保留。
5. **能量光带风格**：通过 Niagara 材质实现（Unlit + Emissive + UV 流动），不在 C++ 侧处理视觉。

## Verification Steps

1. 编译：`Build.bat LiaoBoVREditor Win64 Development -module=MeetVRTemplate -WaitMutex`
2. 不配置 Niagara System 时：确认 ConnectorSpline 的 debug 线仍正常工作（无回归）
3. 配置 Niagara System 后：
   - PIE，确认 ribbon 沿 Spline 点渲染
   - 调整 `SineAmplitude`/`SineSpeed`，确认 ribbon 跟随波形
   - 切换 `WaveMode`，确认行波/驻波下 ribbon 表现正确
   - 配合 GuideSpline，确认 ribbon 沿弯曲路径渲染
4. 检查性能：16-64 个点的数组每帧传输是否流畅

## Niagara System 配置手把手指导

由于用户选择"指导我完成"，执行阶段我会：
1. 先完成所有 C++ 改动并编译
2. 然后在编辑器内逐步指导用户创建 `NS_ConnectorRibbon` 资产（每一步的具体操作、参数名、绑定关系）
3. 如果用户遇到问题，提供 troubleshooting
