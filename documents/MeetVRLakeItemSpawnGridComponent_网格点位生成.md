# Plan: MeetVRLakeItemSpawnGridComponent — 网格点位生成组件（矩形）

## Summary
新建一个**自包含、独立**的网格点位生成组件 `UMeetVRLakeItemSpawnGridComponent`（USceneComponent），用于在湖面（或任意平面）上提供"当前可生成的随机位置"，通过**网格占用追踪**保证随机生成物不重叠。**仅支持矩形网格**（用户已确认取消圆形）。仅交付组件本身，不改动 Manager。

## Current State Analysis
- LakeItem 既有组件均为独立、单一职责、前缀 `MeetVRLakeItem*`：`UMeetVRLakeItemWeightComponent`/`UMeetVRLakeItemSizeComponent`（UActorComponent，纯计算）；`UMeetVRLakeItemMarkerComponent`（USceneComponent，有 transform）。
- 参考 `SuckerActorSpawnerComponent` 的区域采样（矩形 `FRandRange(-Half,Half)`），但**无占用追踪/不防重叠** → 本组件用网格占用补齐。
- Manager 现有 `SpawnMarkerAtLocation(FVector)` 为低层入口，调用方需自备位置 → 本组件用于"取位置"；但按用户决定，本次**不接入 Manager**。
- 文件落点：`Public/LakeItem/` 与 `Private/LakeItem/`，UE 模块自动 glob，无需改 build.cs。

## Proposed Changes

### 文件 1（新建）：`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemSpawnGridComponent.h`

类 `UMeetVRLakeItemSpawnGridComponent : public USceneComponent`，`Blueprintable + BlueprintSpawnableComponent`。

配置（EditAnywhere, BlueprintReadWrite）：
- `FVector2D RectangleSize = (1000,1000)`（X=长, Y=宽，ClampMin=0）
- `FVector2D CellSize = (100,100)`（X,Y，ClampMin=0.1；**防重叠关键**：需 ≥ 物品最大外接尺寸）
- `float HeightOffset = 0`（Z 偏移，相对组件位置）
- `bool bAutoInitOnBeginPlay = true`

运行时状态（Transient, BlueprintReadOnly）：
- `int32 GridCols`, `int32 GridRows`
- `TArray<bool> CellOccupied`（size = Cols*Rows；O(1) 占用查询）
- `TArray<int32> FreeCellIndices`（O(1) 随机抽取 + swap-remove 占用）

公开 API（核心）：
```cpp
// 重新按配置构建网格（清空占用；BeginPlay 自动调一次，或手动调）
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
void InitGrid();

// 取一个随机空闲位置（默认占用）；成功返回 true 并填 OutLocation，无空闲返回 false
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
bool TryGetRandomFreeLocation(FVector& OutLocation, bool bMarkOccupied = true);

// 按世界坐标占用/释放所在格（marker 销毁时由调用方调 Release）
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
bool MarkOccupiedAtLocation(FVector WorldLocation);
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
bool ReleaseLocation(FVector WorldLocation);

UFUNCTION(BlueprintPure, Category="MeetVR|LakeItem|SpawnGrid")
int32 GetFreeCellCount() const;          // 当前可生成数
UFUNCTION(BlueprintPure, Category="MeetVR|LakeItem|SpawnGrid")
int32 GetTotalCellCount() const;         // Cols*Rows
UFUNCTION(BlueprintPure, Category="MeetVR|LakeItem|SpawnGrid")
bool IsLocationOccupied(FVector WorldLocation) const;
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
void ResetOccupancy();                   // 全部占用复位为空闲
UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|SpawnGrid")
TArray<FVector> GetAllFreeLocations() const; // 可视化/批处理
```

protected：
```cpp
virtual void BeginPlay() override;
int32 CellIndexFromXY(int32 Col, int32 Row) const;       // Col + Row*Cols
int32 LocationToCellIndex(FVector WorldLocation) const;  // -1=越界
FVector CellIndexToLocation(int32 CellIndex) const;       // 格中心世界坐标
```

### 文件 2（新建）：`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/LakeItem/MeetVRLakeItemSpawnGridComponent.cpp`

构造：`PrimaryComponentTick.bCanEverTick = false`。

**InitGrid() 算法**：
1. `Cols = max(1, Floor(RectangleSize.X / CellSize.X))`，`Rows = max(1, Floor(RectangleSize.Y / CellSize.Y))`
2. `CellOccupied` Init 为 `Cols*Rows` 个 false
3. `FreeCellIndices` 清空后填入 `[0 .. Cols*Rows-1]`

**CellIndexToLocation(idx)**：
- `Col = idx % Cols; Row = idx / Cols;`
- 局部偏移（以组件位置为中心）：`lx = (Col + 0.5 - Cols/2) * CellSize.X`，`ly = (Row + 0.5 - Rows/2) * CellSize.Y`
- `World = GetComponentLocation() + GetComponentRotation().RotateVector(FVector(lx, ly, HeightOffset))`

**LocationToCellIndex(loc)**：
- `local = GetComponentRotation().GetInverse().RotateVector(loc - GetComponentLocation())`
- `Col = Floor(local.X / CellSize.X + Cols/2)`，`Row = Floor(local.Y / CellSize.Y + Rows/2)`
- `Col/Row` 越界（<0 或 >=Cols/Rows）返回 -1

**TryGetRandomFreeLocation**：
- 若 `FreeCellIndices.Num()==0` 返回 false
- `Pick = FMath::RandRange(0, FreeCellIndices.Num()-1)`，`Cell = FreeCellIndices[Pick]`
- `OutLocation = CellIndexToLocation(Cell)`
- 若 `bMarkOccupied`：`FreeCellIndices[Pick] = FreeCellIndices.Last(); FreeCellIndices.Pop(); CellOccupied[Cell] = true`
- 返回 true

**MarkOccupiedAtLocation**：`idx = LocationToCellIndex`；idx>=0 且 `!CellOccupied[idx]` → `CellOccupied[idx]=true`，从 FreeCellIndices swap-remove 该 idx；返回是否成功。

**ReleaseLocation**：`idx = LocationToCellIndex`；idx>=0 且 `CellOccupied[idx]` → `CellOccupied[idx]=false`，`FreeCellIndices.Add(idx)`；返回是否成功。

**ResetOccupancy**：`CellOccupied` 全置 false，重建 FreeCellIndices `[0..Cols*Rows-1]`。

**BeginPlay**：`Super::BeginPlay()`；若 `bAutoInitOnBeginPlay` 调 `InitGrid()`。

## Assumptions & Decisions
- **防重叠机制**：网格占用追踪，O(1) 取位/释放。`CellSize` 由用户设为 ≥ 物品最大外接尺寸即可保证不重叠（组件不感知物品实际尺寸，符合单一职责）。
- **形状**：仅矩形（用户已确认取消圆形）。`CellOccupied` 为 bool（0/1），无 Excluded 状态。
- **独立组件**：不改动 Manager。调用方（Manager/BP/其他类）负责：`TryGetRandomFreeLocation` 取位 → `SpawnMarkerAtLocation(位)` → marker 销毁时调 `ReleaseLocation(位)`。本组件不持有任何 actor 引用、不绑定 OnDestroyed。
- **坐标系**：区域定义在组件局部 XY 平面、以组件位置为中心；Z=HeightOffset。支持组件旋转摆向。
- **Tick**：关闭，纯按需调用。
- **重入安全**：`TryGetRandomFreeLocation` 默认占用；同一格不会重复给出。
- **命名/前缀**：遵循 LakeItem 既有约定（`MeetVRLakeItem*` + `UMeetVR...`），不使用 `MeetVRSubtitle` 前缀（那是字幕插件的约定）。

## Verification
1. 编译 MeetVRTemplate 模块（环境就绪后）：确认无链接错误（USceneComponent/BeginPlay/各 UFUNCTION）。
2. BP 内挂到测试 Actor：设 RectangleSize=1000x1000、CellSize=200；循环 `TryGetRandomFreeLocation`，断言每次返回位置互不重叠（间距 ≥ 200）且总数 = 25；`GetFreeCellCount` 递减至 0 后返回 false。
3. 释放某位置后 `GetFreeCellCount` +1；`ResetOccupancy` 后 `GetFreeCellCount` == `GetTotalCellCount`。
