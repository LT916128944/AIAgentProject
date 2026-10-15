# Connector 表现层（Visualization Layer）方案分析与实现计划

## 1. 现状分析

当前的 `AMeetVRTentacleConnector` 仅包含 `USplineComponent`。
- `USplineComponent` 是一个**仅用于编辑器/调试**的组件，它本身不具备在游戏运行时（PIE/Shipping）渲染几何体的功能。
- **必须添加表现层**才能让玩家看到触手/连线。

## 2. 表现层方案分析

针对本项目（动态、正弦、可能剧烈形变的路径），主要有以下 4 种方案：

### 方案 A：动态网格生成 (Procedural Mesh Component)
- **原理**：使用 `UDynamicMeshComponent`（新）或 `UPrimitiveComponent`，在 `Tick` 中根据 Spline 的点动态计算顶点（圆形截面）、生成三角面、提交到渲染线程。
- **优点**：完全程序化，无需美术资产。支持任意点数、任意形状。可实现平滑的圆形管状、粗细变化、材质任意变换。
- **缺点**：代码复杂（需手写顶点缓冲/三角索引），每帧 CPU 开销较高（几何计算+提交），需要处理资源释放。

### 方案 B：SplineMesh 组件池 (SplineMeshComponent Pool)
- **原理**：预先创建一个由 N 个 `USplineMeshComponent` 组成的数组（Pool）。每帧将 `ConnectorSpline` 的点分段，为每个 `SplineMesh` 设置起止位置/切线。
- **优点**：实现极其简单，UE 原生支持，支持任意静态网格（可以用球形、胶囊、任意模型段拼接）。
- **缺点**：点数固定，当路径剧烈弯曲时段与段之间会出现间隙或重叠（除非使用极高精度的网格）。性能随池大小线性增长。

### 方案 C：Niagara 粒子系统 (Ribbon Renderer)
- **原理**：每帧将 Spline 的点作为 Spawn Location 传给 Niagara，使用 Ribbon 渲染模式生成带状连线。
- **优点**：无需 C++ 网格代码，性能极佳，天然支持发光、混合、拖尾。
- **缺点**：视觉效果是"光带/能量线"而非实体触手，且需要美术在 Niagara 中配置，不适合作为纯代码插件的默认方案。

### 方案 D：控制静态网格形变
- **原理**：准备一个细长的触手 Static Mesh，通过世界位置偏移（Material World Position）或骨骼动画让它"贴合" Spline。
- **优点**：美术表现最佳，可结合骨骼动画做精细的蠕动、挤压。
- **缺点**：必须依赖美术资产（一个特定的触手模型），且动态形变有限（极度弯曲时可能穿模）。不适合作为插件的通用默认方案。

## 3. 推荐方案

推荐 **方案 B（SplineMesh 组件池）** 作为基础实现：
- **代码最简单、维护成本最低**，能快速得到正确的视觉表现。
- **可扩展性好**：后续可无缝切换到方案 A（Procedural Mesh），接口不变。
- **灵活度高**：用户可以指定任意网格（胶囊、圆柱、球体）来拼接触手外观。

## 4. 实现计划（基于方案 B）

### 4.1 新增渲染参数（.h 文件）
- **`ConnectorMesh`** (`UStaticMesh*`)：用户指定的网格资产（默认使用引擎自带的圆柱或胶囊 Mesh）。
- **`MeshBaseScale`** (`FVector`)：网格基础缩放，用于调整粗细。
- **`bShowConnectorMesh`** (`bool`)：总开关，允许运行时隐藏表现层。

### 4.2 新增辅助函数（.h / .cpp 文件）
- **`InitMeshPool()`**：初始化 `TArray<USplineMeshComponent*>`，预分配足够数量的组件。
- **`UpdateMeshFromSpline()`**：核心函数，在 Tick 中调用。
    - 每帧清空上一帧的 Mesh 状态。
    - 根据 `ConnectorSpline->GetNumberOfSplinePoints()` 确定需要的网格数量。
    - 循环遍历 Spline 点，为每个段 `SetStartAndEnd(StartLoc, StartTan, EndLoc, EndTan)`。

### 4.3 修改流程（.cpp 文件）
- **构造函数**：初始化 MeshPool 数组。
- **Tick 函数**：在 `UpdateSineSpline` 之后，调用 `UpdateMeshFromSpline` 刷新表现。

## 5. 后续可扩展方案（留给未来）
- 如果当前方案（SplineMesh）在剧烈运动下出现视觉断裂，可将 `UpdateMeshFromSpline` 内部的实现替换为 `UDynamicMeshComponent` 的动态几何生成。对外接口保持不变。
