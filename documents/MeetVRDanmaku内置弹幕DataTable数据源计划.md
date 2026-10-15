# MeetVRDanmaku 内置弹幕（DataTable 数据源）实施计划

## 一、Summary / 摘要

在现有 `MeetVRDanmaku` 插件（外部弹幕：HTTP 下载 → 内存数据库 → LevelManager 批量生成）的基础上，新增**内置弹幕**能力：通过 UE DataTable 资产在场景中批量生成弹幕。

已确认设计（用户决策）：
1. **复用现有数据库 + LevelManager**：给 `UMeetVRDanmakuDatabaseSubsystem` 新增 `LoadFromDataTable` 入库 API；`AMeetVRDanmakuLevelManager` 新增可选 `UDataTable* DataTable` 资产属性，设置后优先从 DataTable 加载数据入库，再走现有统一查询/生成流程。
2. **资产引用在 LevelManager 属性**：关卡中把 DataTable 资产拖入 LevelManager 即可；未设置则走原外部弹幕路径，行为完全不变。
3. **批量生成全部**：复用 `SpawnAll()` 语义，一次生成 DataTable 中匹配（SpaceId/SectionId）的全部行。

核心可行性依据：`FMeetVRDanmakuItem` **已继承 `FTableRowBase`**（`MeetVRDanmakuTypes.h` L29），可直接作为 DataTable 行结构使用，无需新增行结构类型。

设计原则（延续既有约定）：数据层完整、C++ 最小侵入、不冗余、不依赖其他系统、不改动用户已有代码结构。

## 二、Current State Analysis / 现状分析

### 现有外部弹幕链路（保留不动）

```
HTTP GET(DefaultServerURL) → ParseJsonResponse → AddBatch → UMeetVRDanmakuDatabaseSubsystem(内存库)
    → AMeetVRDanmakuLevelManager::SpawnAll() → Query(SpaceId/SectionId) → 按 Timestamp 排序 → SpawnActor 批量生成
```

### 关键文件与可复用点

| 文件 | 现状要点 | 本次改动 |
|------|---------|---------|
| `Public/MeetVRDanmakuTypes.h` | `FMeetVRDanmakuItem : FTableRowBase`（Id/SpaceId/SectionId/Content/Color/Priority/Location/Rotation/bHasLocation/Timestamp...） | **不改** |
| `Public/MeetVRDanmakuDatabaseSubsystem.h` + `Private/MeetVRDanmakuDatabaseSubsystem.cpp` | `UGameInstanceSubsystem` 内存库（CRUD + HTTP 下载 + `GetNext` 游标）；`AddBatch` 已支持自动生成 Id、跳过无效行、广播 `OnDatabaseChanged` | **新增** `LoadFromDataTable` |
| `Public/Actors/MeetVRDanmakuLevelManager.h` + `Private/Actors/MeetVRDanmakuLevelManager.cpp` | `AActor` 关卡级单例；`SpawnAll()` 查库批量生成；数据库为空时自动触发 HTTP 下载（`bWaitingForDownload` 防重入）；按 `SpaceId/SectionId` 过滤 + `Timestamp` 升序 | **新增** `DataTable` 属性 + SpawnAll 加载分支 |
| `MeetVRDanmaku.Build.cs` | 已依赖 `Core/CoreUObject/Engine/GameplayTags/HTTP/JsonUtilities/Json/DeveloperSettings` | **不改**（`UDataTable` 属 Engine 模块） |

### 项目 DataTable 用法参考（Phase 1 探索）

- `UMeetVRLakeItemWeightComponent`（`Plugins/MeetVRTemplate/.../MeetVRLakeItemWeightComponent.cpp`）：持有 `TObjectPtr<UDataTable>`，用 `Table->GetRowMap()` + `reinterpret_cast<const FRowStruct*>(Pair.Value)` 遍历行 —— 本次采用同样遍历方式。
- 行结构均继承 `FTableRowBase`，与 `FMeetVRDanmakuItem` 一致。

## 三、Proposed Changes / 提案变更

### 1. `Public/MeetVRDanmakuDatabaseSubsystem.h` — 新增数据层 API

在 `// ===== 增（Create） =====` 区段（`AddBatch` 之后）新增：

```cpp
/**
 * 从 DataTable 资产加载弹幕数据入库（内置弹幕数据源）。
 * 行结构须为 FMeetVRDanmakuItem（已继承 FTableRowBase，编辑器可直接以该结构创建 DataTable）。
 * 空行/无效行跳过；空 Id 由 AddBatch 自动生成。
 * bClearFirst=true 时先 Clear() 再入库（默认 true，保证内置源独立成库；false 可追加合并外部数据）。
 * SpaceId/SectionId 非空时覆盖行内同名字段（便于行内不重复填写，由 LevelManager 统一指定）。
 * 返回成功加载行数；Table 无效/无有效行返回 0。
 */
UFUNCTION(BlueprintCallable, Category = "MeetVR|Danmaku|Database")
int32 LoadFromDataTable(const UDataTable* Table, bool bClearFirst = true, const FString& SpaceId = TEXT(""), const FString& SectionId = TEXT(""));
```

需前向声明 `class UDataTable;`（或包含 `Engine/DataTable.h`，推荐 cpp 中包含、头文件前向声明）。

### 2. `Private/MeetVRDanmakuDatabaseSubsystem.cpp` — 实现 `LoadFromDataTable`

顶部新增 include：`#include "Engine/DataTable.h"`。

实现（仿 `MeetVRLakeItemWeightComponent.cpp` 的 GetRowMap 遍历）：

```cpp
int32 UMeetVRDanmakuDatabaseSubsystem::LoadFromDataTable(const UDataTable* Table, bool bClearFirst, const FString& SpaceId, const FString& SectionId)
{
	if (!Table)
	{
		return 0;
	}

	TArray<FMeetVRDanmakuItem> Items;
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FMeetVRDanmakuItem* Row = reinterpret_cast<const FMeetVRDanmakuItem*>(Pair.Value);
		if (!Row || !Row->IsValidItem())
		{
			continue;
		}
		FMeetVRDanmakuItem Copy = *Row;
		if (!SpaceId.IsEmpty())
		{
			Copy.SpaceId = SpaceId;
		}
		if (!SectionId.IsEmpty())
		{
			Copy.SectionId = SectionId;
		}
		Items.Add(Copy);
	}

	if (Items.Num() == 0)
	{
		return 0;
	}

	if (bClearFirst)
	{
		Clear();
	}
	return AddBatch(Items); // 复用：自动生成空 Id、广播 OnDatabaseChanged
}
```

要点：
- `GetRowMap()` 顺序不保证，但 `SpawnAll()` 内部会按 `Timestamp` 升序排序，时序正确性不受影响。
- 复用 `AddBatch`（去重/自动 Id/无效跳过/广播），不新增冗余逻辑。
- 无需处理 `OnDatabaseChanged`，`AddBatch`/`Clear` 已广播。

### 3. `Public/Actors/MeetVRDanmakuLevelManager.h` — 新增内置数据源属性

在 `// ===== 查询过滤 =====` 区段之前（或之后）新增 `Source` 分组：

```cpp
// ===== 内置数据源（DataTable，可选） =====

/**
 * 内置弹幕数据源（可选）。设置后 SpawnAll 优先从该 DataTable 加载数据入库（复用数据库查询/生成流程）；
 * 未设置则走原外部弹幕路径（数据库为空时触发 HTTP 下载）。
 * 行结构须为 FMeetVRDanmakuItem（已继承 FTableRowBase）。
 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Source")
TObjectPtr<UDataTable> DataTable;

/** 内置数据源：SpawnAll 加载前是否先清空数据库（默认 true，保证内置源独立成库） */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Source")
bool bClearBeforeDataTableLoad = true;
```

需前向声明 `class UDataTable;`（头文件现有 `class UMeetVRDanmakuDatabaseSubsystem;` 附近）。

### 4. `Private/Actors/MeetVRDanmakuLevelManager.cpp` — SpawnAll 增加内置分支

改动 `SpawnAll()` 中"数据库为空触发下载"段落：当 `DataTable` 已设置时，跳过 HTTP 下载分支，改为从 DataTable 加载入库。

**改动前**（`MeetVRDanmakuLevelManager.cpp` L63-77）：

```cpp
	// 数据库为空时触发一次下载，下载完成后再重入 SpawnAll
	if (!DB->HasData())
	{
		if (bWaitingForDownload)
		{
			// 已在等待下载，避免重复触发
			return 0;
		}
		UE_LOG(LogTemp, Log, TEXT("MeetVRDanmakuLevelManager: Database empty, triggering download from default URL..."));
		bWaitingForDownload = true;
		DB->OnDownloadSucceeded.AddDynamic(this, &AMeetVRDanmakuLevelManager::HandleDownloadSucceeded);
		DB->OnDownloadFailed.AddDynamic(this, &AMeetVRDanmakuLevelManager::HandleDownloadFailed);
		DB->RequestDanmakuBatchFromDefaultURL();
		return 0;
	}
```

**改动后**：

```cpp
	// 内置数据源（DataTable）：优先加载入库，再走统一查询生成流程
	if (DataTable)
	{
		const int32 Loaded = DB->LoadFromDataTable(DataTable, bClearBeforeDataTableLoad, SpaceId, SectionId);
		UE_LOG(LogTemp, Log, TEXT("MeetVRDanmakuLevelManager: Loaded %d items from DataTable."), Loaded);
	}
	// 外部数据源（HTTP）：数据库为空时触发一次下载，下载完成后再重入 SpawnAll
	else if (!DB->HasData())
	{
		if (bWaitingForDownload)
		{
			// 已在等待下载，避免重复触发
			return 0;
		}
		UE_LOG(LogTemp, Log, TEXT("MeetVRDanmakuLevelManager: Database empty, triggering download from default URL..."));
		bWaitingForDownload = true;
		DB->OnDownloadSucceeded.AddDynamic(this, &AMeetVRDanmakuLevelManager::HandleDownloadSucceeded);
		DB->OnDownloadFailed.AddDynamic(this, &AMeetVRDanmakuLevelManager::HandleDownloadFailed);
		DB->RequestDanmakuBatchFromDefaultURL();
		return 0;
	}
```

要点：
- `if (DataTable)` 分支：每次 `SpawnAll()` 都重新加载（`bClearBeforeDataTableLoad=true` 时先清库），保证编辑器中修改 DataTable 后重新生成即生效，且可重复 `SpawnAll()`。
- 加载后继续走下方 `Query(SpaceId/SectionId)` → 排序 → 批量生成，**生成逻辑零改动**。
- 空 DataTable（Loaded=0）时自然落到 Query=0 → 广播 `OnSpawnAllComplete(0)`，行为清晰无异常。
- `DataTable` 未设置时分支结构与原逻辑完全一致，外部弹幕行为不变。

### 不改动的文件

- `MeetVRDanmakuTypes.h`（行结构已就绪）
- `Components/MeetVRDanmakuComponent.h/.cpp`、`Actors/MeetVRDanmakuActor.h/.cpp`、`Components/MeetVRDanmakuSpawnComponent.h/.cpp`（若存在）
- `MeetVRDanmakuModule.h/.cpp`
- `MeetVRDanmaku.Build.cs`（`UDataTable` 属 Engine 模块，已依赖）
- `MeetVRDanmaku.uplugin`、`Config/DefaultMeetVRDanmaku.ini`

## 四、数据流（端到端）

1. 开发者新建 DataTable 资产（行结构选 `FMeetVRDanmakuItem`），填写行数据（Content/Speaker/Color/Priority/Location/Rotation/bHasLocation/Timestamp 等；SpaceId/SectionId 可留空由 LevelManager 统一填充）。
2. 关卡中放置 `AMeetVRDanmakuLevelManager`（或其 BP 子类），设置 `DataTable` 资产、`SpaceId`/`SectionId`、`DanmakuActorClass` 等生成配置。
3. BeginPlay（或 BP 手动调 `SpawnAll()`）→ 管理器检测到 `DataTable` → `DB->LoadFromDataTable(Table, bClearFirst, SpaceId, SectionId)` 入库 → `Query` 过滤 → `Timestamp` 升序 → `SpawnOne` 批量生成弹幕载体 Actor（挂 `UMeetVRDanmakuComponent` 写 `SpawnData`）→ 广播 `OnSpawnAllComplete(Count)`。
4. 开发者既有 BP 弹幕 Actor 子类照常读取 `SpawnData` 渲染/飞行，**无需任何改动**。

**运行时旁路**：BP 也可直接调 `GetSubsystem<UMeetVRDanmakuDatabaseSubsystem>()->LoadFromDataTable(...)` 手动入库，再走 `SpawnComponent`/`GetNext` 逐条触发路径（本次虽以批量为主，但数据层 API 天然兼容逐条触发）。

## 五、Assumptions & Decisions / 假设与决策

1. **复用而非新建**：DataTable 数据统一进 `UMeetVRDanmakuDatabaseSubsystem` 内存库，生成走现有 `SpawnAll()`/`Query` 流程，无平行系统、无冗余生成逻辑。
2. **行结构零新增**：`FMeetVRDanmakuItem` 已是 `FTableRowBase`，直接作为 DataTable 行结构；不新增 `FMeetVRBuiltInDanmakuItem` 之类重复类型。
3. **LevelManager 属性承载资产**：`DataTable` 为 `EditAnywhere` 实例属性，关卡内直观配置；未设置 = 原外部弹幕行为不变（向后兼容）。
4. **SpaceId/SectionId 覆盖规则**：`LoadFromDataTable` 的 SpaceId/SectionId 参数非空时覆盖行内值；LevelManager 传自己的 SpaceId/SectionId，行内可不重复填写，也支持行内自带值（LevelManager 留空=通配）。
5. **bClearFirst 默认 true**：内置源独立成库，避免与外部数据混库；需混合时置 false（追加）。
6. **空表处理**：无有效行返回 0，SpawnAll 广播 0，不报错。
7. **顺序**：`GetRowMap()` 无序，但 `SpawnAll()` 已按 `Timestamp` 升序，时序正确。
8. **错误处理边界**：仅在系统边界（DataTable 有效性、行有效性）校验；内部复用 `AddBatch` 既有逻辑。

## 六、Verification / 验证步骤

### 静态检查

1. `GetDiagnostics` 检查 4 个改动文件（`MeetVRDanmakuDatabaseSubsystem.h/.cpp`、`MeetVRDanmakuLevelManager.h/.cpp`）无 IDE 报错。
2. 确认 `LoadFromDataTable` 头文件声明与 cpp 实现签名一致；`UDataTable` 前向声明/包含齐全。

### 编译验证

```powershell
dotnet "F:\EpicGames\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" LiaoBoVREditor Win64 Development -project="F:\LiaoBo_VR\LiaoBoVR.uproject" -NoHotReload -NoLog
```

预期：MeetVRDanmaku 模块编译通过，无 error。

### 功能验证（编译后 BP 流程）

1. 创建 DataTable 资产（行结构 `FMeetVRDanmakuItem`），填入 3+ 行弹幕（含 1 行空 Content 验证跳过）。
2. 关卡放置 `AMeetVRDanmakuLevelManager`（或现有 BP 子类），`DataTable` 设为该资产，`DanmakuActorClass` 设为现有弹幕 BP 载体类，设置 `SpaceId`/`SectionId`。
3. Play：
   - `bAutoSpawnOnBeginPlay=true` → BeginPlay 自动生成全部有效行对应 Actor（空 Content 行不生成）。
   - 验证生成的 Actor 数量 = 有效行数，且各 Actor 的 `SpawnData.Item` 字段（Content/Color/Priority/Location...）与行一致。
   - 验证 `SpaceId`/`SectionId` 过滤：LevelManager 设不匹配值 → 生成 0 个；匹配 → 全生成；LevelManager 留空（通配）→ 全生成。
4. **向后兼容**：清空 `DataTable` 属性再 Play → 走原 HTTP 下载路径，行为与改动前一致（`DefaultServerURL` 生效）。
5. **重复 SpawnAll**：运行时再调 `SpawnAll()` → 重新加载并生成；`bClearBeforeDataTableLoad=false` 时可验证与外部数据追加共存。
6. **BP 旁路**：BP 直接调 `LoadFromDataTable(Table, true)` → 返回行数，`OnDatabaseChanged` 广播，随后 `SpawnComponent` 逐条 `GetNext` 可消费。
