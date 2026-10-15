# 湖面标志下载/缓存 Subsystem 实现计划

## 摘要

为湖面标志（LakeItem）新增一个 `UGameInstanceSubsystem`，仿照 `MeetVRDanmaku` 插件的 `UMeetVRDanmakuDatabaseSubsystem` 范式：通过 HTTP 从文件系统下载 JSON 数据、缓存到内存、提供按需只读查询。仅做数据层，不触碰现有 `WeightComponent`/`SizeComponent`，不负责 `ItemType → ItemClass` 解析（由消费方持 DataTable 映射）。

## 现状分析

### 参考范式（MeetVRDanmaku 插件）
- [UMeetVRDanmakuDatabaseSubsystem](file:///f:/LiaoBo_VR/Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Public/MeetVRDanmakuDatabaseSubsystem.h) — `UGameInstanceSubsystem`，内存数组 `TArray<FMeetVRDanmakuItem>` + `TMap<FString,int32> IdIndex`；HTTP 下载入口 + JSON 解析 + CRUD + Query + GetNext 游标消费 + 三个广播委托。
- [UMeetVRDanmakuDeveloperSettings](file:///f:/LiaoBo_VR/Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Public/MeetVRDanmakuDeveloperSettings.h) — `UDeveloperSettings`（`config=MeetVRDanmaku`），持 `DefaultServerURL` / `bClearBeforeBatch` / `bCyclePool`。
- [AMeetVRDanmakuLevelManager](file:///f:/LiaoBo_VR/Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Private/Actors/MeetVRDanmakuLevelManager.cpp) — 消费方范式：`GetGameInstance->GetSubsystem<...>()` 取库 → `HasData()` 为空则 `RequestDanmakuBatchFromDefaultURL()` + 订阅 `OnDownloadSucceeded` 重入。
- Build.cs 依赖：`HTTP` / `Json` / `JsonUtilities` / `DeveloperSettings`。
- JSON 解析：`TJsonReader` + `FJsonSerializer::Deserialize` 成数组，逐字段 `GetStringField` / `GetNumberField`，`IsValidItem()` 过滤。

### LakeItem 现状（MeetVRTemplate 插件）
- [MeetVRLakeItemTypes.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemTypes.h) — 仅含 `FMeetVRLakeItemEntry`（DataTable 行：`ItemClass` / `Weight` / `ItemCategory`），`#include "Engine/DataTable.h"`。
- [UMeetVRLakeItemWeightComponent](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemWeightComponent.h) — 持 `UDataTable`，`PickRandomEntry()`。
- [AMeetVRLakeItemLevelManager](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/LakeItem/MeetVRLakeItemLevelManager.cpp) — 用 `WeightComponent` 预选 `ItemClass` 写入 marker。
- [UMeetVRLakeItemSizeComponent](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemSizeComponent.h) — `MinSize`/`MaxSize` 正态分布取尺寸。
- [MeetVRTemplate.Build.cs](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/MeetVRTemplate.Build.cs) — 已有 `Json` / `JsonUtilities` / `DeveloperSettings`（Private），**缺 `HTTP`**。
- [Config/DefaultMeetVRTemplate.ini](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Config/DefaultMeetVRTemplate.ini) — 存在，用于追加 DeveloperSettings 段。

## 已确认决策

1. **位置**：放在 MeetVRTemplate 模块内（与 LakeItem 同模块，无需跨模块依赖）。
2. **ItemType 解析**：下载结构中 `ItemType` 存为 `FGameplayTag`；Tag→`TSubclassOf<AMeetVRLakeItem>` 映射由消费方持 DataTable 解析（本计划不实现消费方）。
3. **数据整合**：仅做数据层，不修改 `WeightComponent`/`SizeComponent`/`LevelManager`。
4. **数据格式**：JSON 数组（与弹幕系统一致），字段名 `ID` / `SpaceId` / `ProjectId` / `ItemType` / `SizeMin` / `SizeMax` / `Weight`。
5. **API 范围**：最小集 —— 下载 + 只读查询 + Count/HasData + 三个事件；不做 CRUD/GetNext 游标消费。

## 提议变更

### 1. 扩展数据结构（追加到现有文件，不新建）

**文件**：[MeetVRLakeItemTypes.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemTypes.h)

- 顶部 include 区追加 `#include "GameplayTagContainer.h"`（`FMeetVRLakeItemEntry` 之后追加两个新结构）。
- 追加 `FMeetVRLakeItemData`（`FTableRowBase`）：字段 `Id`(FString) / `SpaceId`(FString) / `ProjectId`(FString) / `ItemType`(FGameplayTag) / `SizeMin`(float,=0.5) / `SizeMax`(float,=2.0) / `Weight`(float,=1.0)；`IsValidItem()` = `!Id.IsEmpty() && ItemType.IsValid() && Weight>0`。
- 追加 `FMeetVRLakeItemQuery`：`SpaceId`(FString,空=通配) / `ProjectId`(FString,空=通配) / `bFilterItemType`(bool) / `ItemType`(FGameplayTag，`EditCondition=bFilterItemType`)。

**理由**：沿用 MeetVRDanmakuTypes.h 把 Item/Query/SpawnData 集中于一文件的约定；避免新建文件；`FMeetVRLakeItemEntry`(DataTable 行) 与 `FMeetVRLakeItemData`(下载数据) 职责不同、命名区分清晰。

### 2. 新增 DeveloperSettings

**新文件**：
- `Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemDeveloperSettings.h`
- `Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/LakeItem/MeetVRLakeItemDeveloperSettings.cpp`

**内容**：
- `UCLASS(config=MeetVRTemplate, defaultconfig, DisplayName="MeetVR LakeItem")` 继承 `UDeveloperSettings`。
- `static const UMeetVRLakeItemDeveloperSettings* Get()`。
- `GetCategoryName()` 返回 `FName("MeetVR")`（与 Danmaku 一致，Project Settings 面板归到 MeetVR 分类）。
- 字段：`DefaultServerURL`(FString,=`"http://localhost:8080/lakeitem"`) / `bClearBeforeBatch`(bool,=false)。
- `.cpp` 仅含空构造（UE 5.x UDeveloperSettings 无需强制实现其他接口）。

**理由**：把 URL/清空策略从 Subsystem 字段解耦，可 Project Settings 配置；`config=MeetVRTemplate` 写入 `DefaultMeetVRTemplate.ini`。

### 3. 新增 DatabaseSubsystem

**新文件**：
- `Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemDatabaseSubsystem.h`
- `Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/LakeItem/MeetVRLakeItemDatabaseSubsystem.cpp`

**头文件公开 API（最小集）**：
- `Initialize` / `Deinitialize` override。
- 查询：`GetById(Id, bFound)` / `Query(Filter)` / `GetAll()` / `Count()` / `HasData()`。
- 下载：`RequestLakeItemBatch(URL)` / `RequestLakeItemBatchFromDefaultURL()` / `IsRequesting()`。
- 委托：`OnDownloadSucceeded`(ItemCount) / `OnDownloadFailed`(Reason) / `OnDatabaseChanged`。

**私有成员**：
- `TArray<FMeetVRLakeItemData> Records` + `TMap<FString,int32> IdIndex`（Id→下标）。
- `ActiveRequestURL` + `bRequestInProgress`（同 URL 在途去重）。
- 辅助：`MatchesQuery` / `RebuildIndex` / `NotifyChanged` / `AddBatchInternal`(private 写入) / `ClearInternal` / `ParseJsonResponse` / `HandleRequestComplete`。
- 不暴露 Add/Remove/Update（最小集，写入仅由下载流程内部触发）。

**实现要点**（逐函数对齐 Danmaku 实现）：
- `Initialize`/`Deinitialize`：空 / 清空 Records+IdIndex+请求状态。
- `GetById`：`IdIndex.Find` → 返回拷贝；未找到 `bFound=false` 返回默认构造。
- `Query`：遍历 `Records`，`MatchesQuery` 过滤。
- `MatchesQuery`：`SpaceId` 非空且不等→false；`ProjectId` 同理；`bFilterItemType` 且 `Item.ItemType != Query.ItemType`→false。
- `GetAll`/`Count`/`HasData`：直接返回。
- `RequestLakeItemBatch(URL)`：URL 空→广播失败；同 URL 在途→直接返回；`FHttpModule::Get().CreateRequest()` + `SetVerb(GET)` + `SetHeader(Accept, application/json)`；`TWeakObjectPtr` 捕获 + `OnProcessRequestComplete` lambda 调 `HandleRequestComplete`；置 `bRequestInProgress=true` + `ActiveRequestURL`。
- `RequestLakeItemBatchFromDefaultURL`：读 `UMeetVRLakeItemDeveloperSettings::Get()->DefaultServerURL` 调上面。
- `HandleRequestComplete`：复位 `bRequestInProgress`；失败→广播 `OnDownloadFailed`；成功→`ParseJsonResponse` → 解析为空广播失败 → `bClearBeforeBatch` 则 `ClearInternal` → `AddBatchInternal` → 广播 `OnDownloadSucceeded(Added)`。
- `AddBatchInternal`：遍历解析结果，跳过 `!IsValidItem()`，`IdIndex` 命中则覆盖、否则追加 + 索引；返回写入数；>0 调 `NotifyChanged`。
- `ClearInternal`：清 Records+IdIndex + `NotifyChanged`。
- `ParseJsonResponse(Body)`：`TJsonReaderFactory::Create` + `FJsonSerializer::Deserialize` 成 `TArray<TSharedPtr<FJsonValue>>`；逐元素取 `AsObject`，读 `ID`/`SpaceId`/`ProjectId`/`ItemType`/`SizeMin`/`SizeMax`/`Weight`；`ItemType` 用 `FGameplayTag::RequestGameplayTag(FName(*Str))` 解析（未注册 Tag 产出 invalid，`IsValidItem` 过滤，可加 `UE_LOG Warning` 提示）；`IsValidItem()` 通过才入结果。

**理由**：完全复用 Danmaku 已验证的下载/解析/缓存骨架，仅字段适配 + 去除 CRUD/GetNext（最小集）。

### 4. 更新 Build.cs 依赖

**文件**：[MeetVRTemplate.Build.cs](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/MeetVRTemplate.Build.cs)

- 在 `PrivateDependencyModuleNames.AddRange(...)` 数组中追加 `"HTTP"`（其余 `Json`/`JsonUtilities`/`DeveloperSettings` 已存在）。
- **不加** `GameplayTags`（已在 `PublicDependencyModuleNames`）。

### 5. 更新配置 ini

**文件**：[Config/DefaultMeetVRTemplate.ini](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Config/DefaultMeetVRTemplate.ini)

- 文件末尾追加：
  ```
  [/Script/MeetVRTemplate.MeetVRLakeItemDeveloperSettings]
  DefaultServerURL="http://localhost:8080/lakeitem"
  bClearBeforeBatch=False
  ```

## 假设与边界

- 服务器返回顶层 JSON 数组，对象字段名严格为 `ID`/`SpaceId`/`ProjectId`/`ItemType`/`SizeMin`/`SizeMax`/`Weight`（与 Danmaku 严格匹配服务器字段风格一致）。字段名不符时该条目被跳过。
- `ItemType` 字符串必须是已注册的 GameplayTag（项目需在 GameplayTags 管理器或 ini 中登记）；未注册的 Tag 产出 invalid 并被 `IsValidItem` 过滤，附带 `UE_LOG Warning`。
- 本计划不实现消费方集成（LevelManager 改造、Tag→Class 映射 DataTable、SizeComponent 取数据来源切换），属后续任务。
- 不做网络重试/超时/断点续传（Danmaku 亦未做，保持对齐）。
- Subsystem 为 `UGameInstanceSubsystem`，跨关卡存活、GameInstance 生命周期；Level 切换不丢缓存（与 Danmaku 一致）。

## 验证步骤

1. 编译 MeetVRTemplate 模块通过（UHT 生成 `.generated.h` 无报错，HTTP 依赖解析正常）。
2. 启动编辑器 → Project Settings → MeetVR 分类下出现 "MeetVR LakeItem" 页，`DefaultServerURL`/`bClearBeforeBatch` 可编辑。
3. Blueprint 中 `GetSubsystem(MeetVRLakeItemDatabaseSubsystem)` 节点可调 `RequestLakeItemBatchFromDefaultURL` / `Query` / `HasData` / `Count`，委托 `OnDownloadSucceeded` 可绑定。
4. 用本地 mock 服务器返回 JSON 数组（含至少 1 条合法 + 1 条非法 ItemType）→ 调下载 → `OnDownloadSucceeded` 广播且 `Count()` 正确；`Query(SpaceId=xxx)` 返回该场景条目；未注册 Tag 条目被过滤且 Output Log 出现 Warning。
5. `bClearBeforeBatch=true` 时二次下载后旧数据清空（`Count` == 本次写入数）；`=false` 时累加/覆盖。
6. 同 URL 重复调用 `RequestLakeItemBatch` 不触发重复请求（`IsRequesting()` 为 true 期间忽略）。
7. 关卡切换后 Subsystem 缓存仍存在（GameInstance 生命周期）。
