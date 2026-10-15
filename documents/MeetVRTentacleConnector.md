# MeetVRTentacleConnector 实现方案

## 概要

在 `MeetVRTemplate` 插件下新增一个连接器 Actor `AMeetVRTentacleConnector`，用于连接"敌人母体"与"敌人触手"两个对象。连接器通过接口 `IMeetVRConnectorEndpoint` 解耦端点引用，承担两项职责：

1. **驱动触手端点运动**：按"基础曲线 + 正弦扰动"组合计算触手目标位置并每帧驱动。
2. **渲染波动连接体**：用 `USplineComponent` + `USplineMeshComponent` 沿母体→触手生成连接体，叠加轴向蠕动正弦扰动。

正弦方向默认为轴向蠕动（沿连接长度方向呈正弦收缩/传播），可配置切换为横向/螺旋。

***

## 现状分析

* **插件位置**：`f:\LiaoBo_VR\Plugins\MeetVRTemplate\`，单模块 `MeetVRTemplate`，Runtime，PreDefault 加载。

* **命名约定**：C++ 类统一 `MeetVR` 前缀（如 `AMeetVRAbilityActorBase`、`UMeetVRPlayerInfoComponent`）；`ASuckerWeaponBase` 是少数不带前缀的特例。本方案遵循 `MeetVR` 前缀。

* **目录约定**：`Source/MeetVRTemplate/Public/<功能>/` 与 `Private/<功能>/` 分层，如 `Sucker/`、`Character/`、`Weapons/`。

* **Actor 模式参考**：[SuckerWeaponBase.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Sucker/SuckerWeaponBase.h) 与 [SuckerWeaponBase.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Sucker/SuckerWeaponBase.cpp) —— 构造函数用 `CreateDefaultSubobject` + `SetupAttachment`，组件用 `TObjectPtr`，UCLASS 标 `Blueprintable/BlueprintType`，委托用 `DECLARE_DYNAMIC_MULTICAST_DELEGATE`。

* **敌人系统现状**：项目中**不存在** Enemy/Mother/Tentacle/Boss 任何类，也无 Spline/Cable 使用先例。因此连接器必须通过接口解耦，不硬依赖任何敌人类。

* **Build.cs 依赖**：[MeetVRTemplate.Build.cs](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/MeetVRTemplate.Build.cs) 已含 `"Engine"` 模块，`USplineComponent`/`USplineMeshComponent` 均属于 Engine 模块，**无需新增模块依赖**。

* **用户偏好**：命名带 `MeetVR` 前缀；独立功能、不依赖现有系统；数据层完整优先（蓝图可扩展）。

***

## 设计决策（已与用户确认）

| 决策点  | 选择                            | 说明                                                   |
| ---- | ----------------------------- | ---------------------------------------------------- |
| 实现形式 | Actor                         | `AMeetVRTentacleConnector : AActor`，独立放置/销毁          |
| 端点引用 | 接口 `IMeetVRConnectorEndpoint` | 母体/触手类实现该接口提供锚点变换                                    |
| 渲染表现 | SplineMesh + 正弦扰动             | `USplineComponent` 生成曲线，`USplineMeshComponent` 沿样条变形 |
| 运动主体 | 两者都要                          | 既驱动触手位置，又渲染波动连接体                                     |
| 正弦方向 | 轴向蠕动（默认，可配置）                  | 沿连接轴向呈正弦收缩/传播                                        |

***

## 新增文件清单

新建 `Connector` 功能目录：

| 文件        | 路径                                              | 作用           |
| --------- | ----------------------------------------------- | ------------ |
| 接口        | `Public/Connector/IMeetVRConnectorEndpoint.h`   | 端点接口，母体/触手实现 |
| Actor 头文件 | `Public/Connector/MeetVRTentacleConnector.h`    | 连接器 Actor 声明 |
| Actor 实现  | `Private/Connector/MeetVRTentacleConnector.cpp` | 连接器逻辑实现      |

即：

* `f:\LiaoBo_VR\Plugins\MeetVRTemplate\Source\MeetVRTemplate\Public\Connector\IMeetVRConnectorEndpoint.h`

* `f:\LiaoBo_VR\Plugins\MeetVRTemplate\Source\MeetVRTemplate\Public\Connector\MeetVRTentacleConnector.h`

* `f:\LiaoBo_VR\Plugins\MeetVRTemplate\Source\MeetVRTemplate\Private\Connector\MeetVRTentacleConnector.cpp`

**不修改** Build.cs（Engine 模块已含 Spline 相关类）。

***

## 详细设计

### 1. 接口 `IMeetVRConnectorEndpoint`

文件：`Public/Connector/IMeetVRConnectorEndpoint.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IMeetVRConnectorEndpoint.generated.h"

UINTERFACE(BlueprintType)
class UMeetVRConnectorEndpoint : public UInterface
{
    GENERATED_BODY()
};

/**
 * 连接器端点接口。
 * 敌人母体、敌人触手类实现此接口，向 MeetVRTentacleConnector 提供连接锚点信息。
 */
class IMeetVRConnectorEndpoint
{
    GENERATED_BODY()
public:
    /** 获取连接锚点的世界变换（位置+朝向）。默认返回 Actor 自身变换。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MeetVR|Connector")
    FTransform GetConnectorAnchorTransform() const;

    /** 可选：连接器驱动触手端点时调用，设置端点目标位置。
     *  默认实现调用 Actor->SetActorLocation。触手类可重写以接入移动组件/物理。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MeetVR|Connector")
    void SetConnectorDrivenLocation(const FVector& NewLocation);
};
```

要点：

* 用 `BlueprintNativeEvent`，C++ 端提供 `_Implementation` 默认实现，蓝图可重写。

* 默认实现：`GetConnectorAnchorTransform` 返回 `Cast<AActor>(this)->GetActorTransform()`；`SetConnectorDrivenLocation` 调用 `Actor->SetActorLocation(NewLocation)`。

* 接口文件需在 cpp 中提供默认 `_Implementation`，或用 `BlueprintNativeEvent` 的默认行为。为简洁，将默认实现放在 `MeetVRTentacleConnector.cpp` 旁的独立 cpp 或直接内联。**决策**：接口默认实现放在新建的 `IMeetVRConnectorEndpoint.cpp`（可选）或在连接器 cpp 中通过 `IIMeetVRConnectorEndpoint::Execute_*` 调用。为避免接口无 cpp 导致链接错误，**新增第 4 个文件** `Private/Connector/IMeetVRConnectorEndpoint.cpp` 提供 `_Implementation` 默认实现。

修订文件清单（增加接口实现 cpp）：

* `Private/Connector/IMeetVRConnectorEndpoint.cpp` —— 提供两个方法的默认 `_Implementation`。

### 2. Actor `AMeetVRTentacleConnector`

文件：`Public/Connector/MeetVRTentacleConnector.h`

#### 2.1 继承与宏

```cpp
UCLASS(Abstract, Blueprintable, BlueprintType, ClassGroup = (MeetVR))
class MEETVRTEMPLATE_API AMeetVRTentacleConnector : public AActor
{
    GENERATED_BODY()
public:
    AMeetVRTentacleConnector(const FObjectInitializer& ObjectInitializer);
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;
    virtual bool ShouldTickIfViewportsOnly() const override; // 编辑器预览
};
```

#### 2.2 组件

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
TObjectPtr<USceneComponent> DefaultSceneRoot;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
TObjectPtr<USplineComponent> ConnectorSpline;  // 母体→触手 样条
```

`USplineMeshComponent` 数量随分段动态创建/复用，不作为固定 UPROPERTY 子对象，而在 cpp 中按 `SplineSegments` 动态 `NewObject` 并 Attach。

#### 2.3 端点引用

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoints", meta = (MustImplement = "MeetVRConnectorEndpoint"))
TWeakObjectPtr<AActor> MotherEndpoint;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoints", meta = (MustImplement = "MeetVRConnectorEndpoint"))
TWeakObjectPtr<AActor> TentacleEndpoint;

/** 是否驱动触手端点位置（关闭则仅渲染连接体）。 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Endpoints")
bool bDriveTentaclePosition = true;
```

初始化辅助：

```cpp
UFUNCTION(BlueprintCallable, Category = "MeetVR|Connector")
void InitConnector(AActor* Mother, AActor* Tentacle, bool bDriveTentacle = true);
```

#### 2.4 运动参数（驱动触手端点）

```cpp
/** 基础运动曲线类型。 */
UENUM(BlueprintType)
enum class EMeetVRConnectorMotionCurve : uint8
{
    Circle   UMETA(DisplayName = "Circle (around mother)"),
    Line     UMETA(DisplayName = "Line (back-forth)"),
    Helix    UMETA(DisplayName = "Helix"),
    Bezier   UMETA(DisplayName = "Bezier Arc")
};

/** 正弦扰动方向。 */
UENUM(BlueprintType)
enum class EMeetVRSineDirection : uint8
{
    Axial     UMETA(DisplayName = "Axial (peristalsis)"),   // 默认
    Lateral   UMETA(DisplayName = "Lateral (sway)"),
    Helical   UMETA(DisplayName = "Helical (twist)")
};

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
EMeetVRConnectorMotionCurve MotionCurveType = EMeetVRConnectorMotionCurve::Circle;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion", meta = (ClampMin = "0.0"))
float MotionRadius = 120.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion", meta = (ClampMin = "0.0"))
float MotionSpeed = 1.0f;            // 角速度/速度

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
FVector MotionAxis = FVector::UpVector;  // 圆/螺旋轴向

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion", meta = (ClampMin = "0.0"))
float SineAmplitude = 30.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion", meta = (ClampMin = "0.001"))
float SineFrequency = 2.0f;          // Hz

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
float SinePhase = 0.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion")
EMeetVRSineDirection EndpointSineDirection = EMeetVRSineDirection::Axial;
```

#### 2.5 渲染参数（连接体波动）

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
TObjectPtr<UStaticMesh> ConnectorMesh;   // 用户在编辑器指定

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = "2", ClampMax = "64"))
int32 SplineSegments = 16;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = "0.01"))
float MeshBaseScale = 1.0f;             // SplineMesh 起止缩放

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = "0.0"))
float PeristalsisAmplitude = 0.4f;      // 半径沿轴向正弦变化幅度（相对 MeshBaseScale）

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (ClampMin = "0.0"))
float PeristalsisFrequency = 3.0f;      // 沿轴向波数

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
float PeristalsisSpeed = 1.0f;          // 蠕动波传播速度

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
EMeetVRSineDirection ConnectorSineDirection = EMeetVRSineDirection::Axial;
```

#### 2.6 关键方法

```cpp
// 计算 t 时刻触手端点目标位置（基础曲线 + 正弦扰动）
FVector ComputeTentaclePosition(float Time, const FVector& MotherPos) const;

// 在基础位置上叠加正弦扰动
FVector ApplySinePerturbation(const FVector& BasePos, const FVector& Tangent,
                              float Time, EMeetVRSineDirection Dir) const;

// 重建样条点（母体→触手，中间点按蠕动/扰动）
void UpdateSplinePoints(const FVector& MotherPos, const FVector& TentaclePos, float Time);

// 重建 SplineMesh 段并应用轴向蠕动缩放
void UpdateSplineMeshes(float Time);

// 获取端点锚点位置（通过接口，带 fallback）
bool GetEndpointLocation(const TWeakObjectPtr<AActor>& Endpoint, FVector& OutLoc) const;

// 驱动触手到目标位置（通过接口 SetConnectorDrivenLocation）
void DriveTentacleTo(const FVector& TargetLoc);
```

### 3. Tick 流程（`MeetVRTentacleConnector.cpp`）

```
Tick(DeltaTime):
  if !MotherEndpoint || !TentacleEndpoint: return
  ElapsedTime += DeltaTime

  // 1. 取端点位置
  MotherPos = GetEndpointLocation(MotherEndpoint)
  // 2. 计算触手目标位置（若驱动）
  TargetPos = ComputeTentaclePosition(ElapsedTime, MotherPos)
  if bDriveTentaclePosition:
      DriveTentacleTo(TargetPos)
  TentaclePos = bDriveTentaclePosition ? TargetPos : GetEndpointLocation(TentacleEndpoint)

  // 3. 更新样条 + 网格
  UpdateSplinePoints(MotherPos, TentaclePos, ElapsedTime)
  UpdateSplineMeshes(ElapsedTime)
```

### 4. 核心算法

#### 4.1 `ComputeTentaclePosition`（基础曲线 + 正弦）

以母体位置为基准：

* **Circle**：`BasePos = MotherPos + (cos(θ)·Right + sin(θ)·Forward) * MotionRadius`，`θ = Time * MotionSpeed`，`Right/Forward` 由 `MotionAxis` 构造正交基。

* **Line**：`BasePos = MotherPos + Dir * sin(θ) * MotionRadius`（来回往复）。

* **Helix**：Circle 基础上叠加 `Up * (Time * MotionSpeed * 20)` 轴向上升（可限幅）。

* **Bezier**：从 `MotherPos + StartOffset` 到 `MotherPos + EndOffset` 的二次贝塞尔，`t = fract(Time*MotionSpeed)`。

叠加扰动：`FinalPos = ApplySinePerturbation(BasePos, Tangent, Time, EndpointSineDirection)`

* **Axial**：`FinalPos = BasePos + Tangent * sin(2π·SineFrequency·Time + SinePhase) * SineAmplitude`（沿切向来回，蠕动式非匀速）

* **Lateral**：`FinalPos = BasePos + Normal * sin(...) * SineAmplitude`（法向摆动）

* **Helical**：`FinalPos = BasePos + (Normal*sin + Binormal*cos) * SineAmplitude`（螺旋摆动）

#### 4.2 `UpdateSplinePoints`

* 起点 `Spline->SetSplinePointAt(MotherPos, 0, ESplineCoordinateSpace::World)`

* 终点 `TentaclePos`

* 中间按 `SplineSegments` 采样，根据 `ConnectorSineDirection` 对中间点叠加扰动（与端点扰动独立，用 Peristalsis 参数或共用 Sine 参数——**决策**：连接体形状扰动共用 `SineAmplitude/Frequency`，方向用 `ConnectorSineDirection`）。

* `Spline->UpdateSpline()`。

#### 4.3 `UpdateSplineMeshes`（轴向蠕动核心）

* 维护 `TArray<TObjectPtr<USplineMeshComponent>> MeshPool`，按 `SplineSegments` 数量复用/新建（`NewObject<USplineMeshComponent>(this)`，`RegisterComponent`，`AttachToComponent(ConnectorSpline)`，`SetStaticMesh(ConnectorMesh)`）。

* 对每段 `i`：

  * `StartPos/StartTangent = Spline->GetLocationAndTangentAtSplinePoint(i, ...)`

  * `EndPos/EndTangent = ... (i+1)`

  * `MeshComp->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent, true)`

  * **轴向蠕动缩放**：`Scale = MeshBaseScale + PeristalsisAmplitude * MeshBaseScale * sin(2π·PeristalsisFrequency·(Alpha) - 2π·PeristalsisSpeed·Time)`，其中 `Alpha = i / SplineSegments` 沿轴向的位置比。

  * `MeshComp->SetStartScale(FVector2D(Scale, Scale))` / `SetEndScale(...)`（相邻段共享端点 scale 保证连续）。

* 蠕动波随 `Time` 沿轴向传播 → 视觉为连接体蠕动。

#### 4.4 接口调用

`GetEndpointLocation`：

```cpp
if (Endpoint.IsValid())
{
    if (auto* Iface = Cast<IMeetVRConnectorEndpoint>(Endpoint.Get()))
        return Iface->Execute_GetConnectorAnchorTransform(Endpoint.Get()).GetLocation();
    // fallback：未实现接口则直接取 ActorLocation
    return Endpoint->GetActorLocation();
}
return false;
```

`DriveTentacleTo`：

```cpp
if (TentacleEndpoint.IsValid())
{
    if (auto* Iface = Cast<IMeetVRConnectorEndpoint>(TentacleEndpoint.Get()))
        Iface->Execute_SetConnectorDrivenLocation(TentacleEndpoint.Get(), TargetLoc);
    else
        TentacleEndpoint->SetActorLocation(TargetLoc);
}
```

***

## 假设与决策

1. **接口解耦**：母体/触手类尚不存在，连接器通过 `IMeetVRConnectorEndpoint` 解耦。敌人类后续实现接口即可接入，无需改动连接器。
2. **默认实现 fallback**：端点未实现接口时，回退到 `AActor::GetActorLocation()` / `SetActorLocation()`，保证可用任意 Actor 测试。
3. **驱动开关**：`bDriveTentaclePosition` 默认 true。触手若自带移动逻辑可关闭，或重写 `SetConnectorDrivenLocation` 接入移动组件/物理。
4. **轴向蠕动语义**：连接体渲染的轴向蠕动 = SplineMesh 沿样条各点半径正弦缩放 + 时间相位传播（蠕动波沿轴向流动）。端点运动的轴向扰动 = 沿轨迹切向正弦位移。
5. **网格资源**：`ConnectorMesh` 由用户在编辑器指定，C++ 不提供默认网格（避免硬依赖内容资产）。
6. **网络同步**：本期不做复制（用户未提）。本地/单机优先，后续可加 `GetLifetimeReplicatedProps` 扩展。
7. **编辑器预览**：`ShouldTickIfViewportsOnly() = true`，便于在编辑器中调整参数即时预览。
8. **SplineMesh 池化**：动态创建的 SplineMeshComponent 存入 `MeshPool` 复用，分段数变化时增删，避免每帧重建 UObject。
9. **不修改 Build.cs**：Engine 模块已覆盖 Spline 依赖。

***

## 验证步骤

1. **编译**：重新编译 `MeetVRTemplate` 模块，确认无错误（重点检查接口 `_Implementation` 链接、`USplineMeshComponent` 头文件包含 `#include "Components/SplineMeshComponent.h"`）。
2. **放置测试 Actor**：在场景中放两个普通 `Cube` Actor（暂代母体/触手），再放置 `AMeetVRTentacleConnector`。
3. **指定端点**：`InitConnector` 或编辑器 Details 面板把 MotherEndpoint/TentacleEndpoint 指向两个 Cube。
4. **指定网格**：把 `ConnectorMesh` 设为引擎内置 `Shape_Cylinder` 或任意长条网格。
5. **运行 PIE**：

   * 触手 Cube 应绕母体做圆周运动（默认 Circle）并叠加轴向正弦扰动。

   * 母体到触手之间出现 SplineMesh 连接体，沿轴向呈蠕动波传播。
6. **参数调参**：在 Details 调整 `MotionRadius/SineAmplitude/PeristalsisFrequency/PeristalsisSpeed`，确认实时响应。
7. **方向切换**：把 `EndpointSineDirection`/`ConnectorSineDirection` 切到 Lateral/Helical，确认扰动方向变化。
8. **驱动开关**：关闭 `bDriveTentaclePosition`，手动移动触手 Cube，确认连接体仍跟随渲染。
9. **接口接入**（后续敌人类就绪后）：让敌人触手类继承 `IMeetVRConnectorEndpoint` 并重写 `SetConnectorDrivenLocation` 接入 CharacterMovement，确认驱动平滑无冲突。

***

## 实施顺序（执行阶段建议 TodoList）

1. 创建 `Public/Connector/IMeetVRConnectorEndpoint.h` + `Private/Connector/IMeetVRConnectorEndpoint.cpp`（接口 + 默认实现）。
2. 创建 `Public/Connector/MeetVRTentacleConnector.h`（枚举、组件、属性、方法声明）。
3. 创建 `Private/Connector/MeetVRTentacleConnector.cpp`：

   * 构造函数（DefaultSceneRoot、ConnectorSpline、Tick 启用）。

   * `InitConnector`、`GetEndpointLocation`、`DriveTentacleTo`。

   * `ComputeTentaclePosition` + `ApplySinePerturbation`。

   * `UpdateSplinePoints` + `UpdateSplineMeshes`（含蠕动缩放 + MeshPool）。

   * `Tick` 串联。
4. 编译验证。
5. 编辑器内按"验证步骤"做放置 + PIE 调参测试。

