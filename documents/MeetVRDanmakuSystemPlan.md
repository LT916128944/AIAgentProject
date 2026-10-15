# MeetVRDanmaku 简易弹幕系统 实施计划

## 摘要

新建一个**完全独立**的 UE 插件 `MeetVRDanmaku`，实现简易弹幕系统：场景中存在一个目标点（由开发者挂载组件的任意 Actor 充当），通过组件的 Blueprint 可调用函数触发；触发后从**内存数据库**（含完整增删改查）按 `SpaceId`/`SectionId` 查询取一条，生成一个 C++ 飞行载体 Actor（携带运动/生命周期数据，支持 Item 自带 `Location`/`Rotation` 方位），由 BP 开发者附加 Widget 渲染文字。数据库由 HTTP 批量下载填充，亦支持运行时 BP 直接 CRUD。聚焦数据层完整性，不实现视觉 Widget。

## 当前状态分析

### 已有相关资产（来自 Phase 1 探索）

- **`MeetVRSubtitle` 插件**（`f:\LiaoBo_VR\Plugins\MeetVRSubtitle`）：独立、`EnabledByDefault=false`，含 `FMeetVRSubtitleInfo`、`UMeetVRSubtitleManagerSubsystem`（标签管理单字幕）、`UMeetVRSubtitleWorldComponent`/`UMeetVRSubtitleHeadFollowComponent`（3D 跟随）。其模型为"标签+单条管理"，**与弹幕的"多条同时飞行+运动轨迹"模型不同**，且用户要求新建独立插件，故**不复用其类型**，仅参考其命名/构建规范。
- **HTTP 模式参考**：`MVLHttpBackendService`（`f:\LiaoBo_VR\Plugins\MeetVRLBE\Source\MVLBackend\Private\HttpBackend\MVLHttpBackendService.h`）使用 UE 内置 `HttpModule.h`/`Interfaces/IHttpRequest.h`/`Interfaces/IHttpResponse.h`；`MpyApiClient` 同样使用内置 HTTP。本计划采用 **UE 内置 HTTP 模块**（不依赖 VaRest/MpyApiClient），保证零跨插件依赖。
- **命名规范**：所有类前缀 `MeetVR`，弹幕相关文件前缀 `MeetVRDanmaku`；子系统用 `UGameInstanceSubsystem`；组件 `BlueprintSpawnableComponent`+`Blueprintable`+`ClassGroup=(MeetVR)`（参考 `UMeetVRSubtitleWorldComponent`）。
- **Build.cs 依赖参考**：`MeetVRSubtitle.Build.cs` 仅 `Core/CoreUObject/Engine/GameplayTags`；本插件追加 `HTTP`/`JsonUtilities`/`Json`。
- **GameplayTag 配置格式参考**：`DefaultMeetVRSubtitle.ini` 使用 `+GameplayTagList=(Tag="...",DevComment="")`。

### 用户决策（已确认）

1. 新建独立插件 `MeetVRDanmaku`（不扩展 MeetVRSubtitle）。
2. 触发方式：**纯组件 + BP 函数**（无场景内置碰撞 Actor；组件挂到任意"目标点"Actor，由蓝图/代码调用触发函数）。
3. 数据下载：**批量预下载**（一次性 GET JSON 字幕池存入子系统，触发时从池取一条）。

## 数据契约（服务器 JSON）

服务器返回一个 JSON 数组（`Content-Type: application/json`），每项为对象，字段对应 `FMeetVRDanmakuItem`：

```json
[
  {
    "id": "d-0001",
    "spaceid": "space-Lobby",
    "sectionid": "sec-EntryPoint",
    "delaytime": 0.0,
    "type": "Dialogue",
    "content": "欢迎来到大厅",
    "speaker": "引导员",
    "color": "#FFFFFFFF",
    "priority": 50,
    "lifetime": 3.0,
    "flightspeed": 0.0,
    "fontsize": 0.0,
    "location": { "x": 0, "y": 0, "z": 0 },
    "rotation": { "pitch": 0, "yaw": 0, "roll": 0 },
    "haslocation": false,
    "timestamp": 0,
    "userdata": ""
  }
]
```

字段说明：

| 字段 | JSON 类型 | C++ 字段 | 必填 | 默认 | 说明 |
|---|---|---|---|---|---|
| `id` | string | `FString Id` | 否 | 自动生成 | 弹幕唯一标识；空时数据库自动生成 `d-{GUID}` |
| `spaceid` | string | `FString SpaceId` | 否 | "" | 场景/空间标识，**用于数据库查询筛选** |
| `sectionid` | string | `FString SectionId` | 否 | "" | 段落/触发点标识，**用于数据库查询筛选** |
| `delaytime` | number | `float DelayTime` | 否 | 0.0 | 触发后延迟显示秒数；**当前默认0=立即显示**，字段保留以支持未来节奏 |
| `type` | string | `EMeetVRDanmakuType Type` | 否 | Custom | 枚举：`Dialogue/Narration/System/Custom` |
| `content` | string | `FText Content` | 是 | — | 弹幕文本 |
| `speaker` | string | `FText Speaker` | 否 | 空 | 说话者名 |
| `color` | string | `FLinearColor Color` | 否 | 白不透明 | `#RRGGBBAA` 十六进制，缺省白色 |
| `priority` | int | `int32 Priority` | 否 | 50 | 优先级（竞争时排序，留扩展） |
| `lifetime` | number | `float Lifetime` | 否 | 0.0 | 本条生命周期秒，0=用组件默认 |
| `flightspeed` | number | `float FlightSpeed` | 否 | 0.0 | 本条飞行速度 cm/s，0=用组件默认 |
| `fontsize` | number | `float FontSize` | 否 | 0.0 | 字体缩放，0=用默认（VR 可读性） |
| `location` | object | `FVector Location` | 否 | (0,0,0) | 生成位置；`haslocation=true` 时作为世界坐标使用，否则用组件目标点+SpawnOffset |
| `rotation` | object | `FRotator Rotation` | 否 | (0,0,0) | 生成朝向；`haslocation=true` 时使用，否则由组件按 FlightMode 计算 |
| `haslocation` | bool | `bool bHasLocation` | 否 | false | 是否用 Item 自带方位覆盖组件默认生成位置/朝向 |
| `timestamp` | int | `int64 Timestamp` | 否 | 0 | 服务器时间戳，排序/时序基准 |
| `userdata` | string | `FString UserData` | 否 | "" | 透传 JSON 字符串，服务器扩展不破坏 C++ 结构 |

解析规则：
- `content` 缺失或空的项跳过；其他字段缺失用默认值。
- `type` 字符串无法匹配枚举时回落 `Custom`。
- `color` 解析失败回落白色不透明。
- `location`/`rotation` 对象缺子字段用 0；`haslocation=false` 时二者被忽略。
- 解析失败的单项跳过但不中断整体下载；数组为空或解析全失败时触发 `OnDownloadFailed`。

## 提议变更（文件清单）

新建插件根目录：`f:\LiaoBo_VR\Plugins\MeetVRDanmaku\`

### 1. 插件描述与构建

**`MeetVRDanmaku.uplugin`** — 插件描述，仿 `MeetVRSubtitle.uplugin`。`FriendlyName="MeetVR Danmaku"`，`EnabledByDefault=false`，`CanContainContent=false`，单模块 `MeetVRDanmaku`，`Type=Runtime`，`LoadingPhase=Default`。

**`Source/MeetVRDanmaku/MeetVRDanmaku.Build.cs`** — 依赖：`Core, CoreUObject, Engine, GameplayTags, HTTP, JsonUtilities, Json`。`PCHUsage=UseExplicitOrSharedPCHs`。

### 2. 模块入口

**`Public/MeetVRDanmakuModule.h`** + **`Private/MeetVRDanmakuModule.cpp`** — 空实现 `FMeetVRDanmakuModule : IModuleInterface`，`IMPLEMENT_MODULE`。仿 `MeetVRSubtitleModule`。

### 3. 数据类型（数据契约层）

**`Public/MeetVRDanmakuTypes.h`**（纯头文件 + `.generated.h`）：

- `UENUM(BlueprintType) enum class EMeetVRDanmakuType : uint8`：`Dialogue`（角色对话）、`Narration`（旁白）、`System`（系统提示）、`Custom`（自定义）。
- `UENUM(BlueprintType) enum class EMeetVRDanmakuFlightMode : uint8`：
  - `Upward`（竖直向上飞）
  - `OutwardFromCenter`（以目标点为圆心向外辐射，叠加向上分量）
  - `CustomDirection`（使用组件配置的固定方向）
- `USTRUCT(BlueprintType) struct FMeetVRDanmakuItem`（继承 `FTableRowBase`，便于数据表扩展）：
  - `FString Id`（唯一标识，空时数据库自动生成）
  - `FString SpaceId`（场景/空间标识，数据库查询筛选用）
  - `FString SectionId`（段落/触发点标识，数据库查询筛选用）
  - `float DelayTime=0.0`（触发后延迟显示秒，默认0=立即）
  - `EMeetVRDanmakuType Type=Custom`
  - `FText Content`（必填文本）
  - `FText Speaker`（说话者）
  - `FLinearColor Color=FLinearColor::White`（含 alpha，默认白不透明）
  - `int32 Priority=50`
  - `float Lifetime=0.0`（0=用组件默认）
  - `float FlightSpeed=0.0`（0=用组件默认，cm/s）
  - `float FontSize=0.0`（0=用默认缩放）
  - `FVector Location=FVector::ZeroVector`（生成位置，bHasLocation=true 时使用）
  - `FRotator Rotation=FRotator::ZeroRotator`（生成朝向，bHasLocation=true 时使用）
  - `bool bHasLocation=false`（是否用 Item 自带方位覆盖组件默认）
  - `int64 Timestamp=0`（服务器时间戳）
  - `FString UserData`（透传 JSON）
- `USTRUCT(BlueprintType) struct FMeetVRDanmakuSpawnData`：`FVector SpawnLocation`（最终生成位置，已合并 Item.bHasLocation 覆盖）、`FRotator SpawnRotation`（最终朝向）、`FVector Velocity`（单位 cm/s × 方向）、`float Lifetime`（已合并组件默认与 Item 覆盖后的最终值）、`float DelayTime`、`float SpawnTime`、`FMeetVRDanmakuItem Item`。这是组件→Actor 的数据载荷。
- `USTRUCT(BlueprintType) struct FMeetVRDanmakuQuery`：`FString SpaceId`（空=通配）、`FString SectionId`（空=通配）、`EMeetVRDanmakuType Type`、`bool bFilterType=false`（是否按类型筛选）。用于数据库查询/取下一条的过滤条件。

### 4. 内存数据库子系统（数据层）

**`Public/MeetVRDanmakuDatabaseSubsystem.h`** + **`Private/MeetVRDanmakuDatabaseSubsystem.cpp`** — `UGameInstanceSubsystem`（单例），既是内存数据库（CRUD）又是 HTTP 下载入口，是数据层统一入口。`Initialize`/`Deinitialize` 管理 HTTP 句柄与配置同步。

- 数据结构：`TArray<FMeetVRDanmakuItem> Records` + `TMap<FString, int32> IdIndex`（Id→下标，O(1) 查找/删除/更新）。所有写操作维护二者一致。
- 配置（`EditDefaultsOnly`/`Config`，可被 `DefaultMeetVRDanmaku.ini` 覆盖）：`FString DefaultServerURL`、`bool bCyclePool=true`、`bool bClearBeforeBatch=false`。
- **增（Create）**：
  - `AddItem(UPARAM(ref) FMeetVRDanmakuItem& Item) → FString` — 空 Id 自动生成 `d-{GUID短格式}` 回填，已存在 Id 则覆盖更新；返回最终 Id。
  - `AddBatch(const TArray<FMeetVRDanmakuItem>& Items) → int32` — 批量插入，返回成功条数。
- **删（Delete）**：`RemoveById(const FString& Id) → bool`、`RemoveBySpaceId(const FString&) → int32`、`RemoveBySectionId(const FString&) → int32`、`RemoveByQuery(const FMeetVRDanmakuQuery&) → int32`、`Clear()`。
- **改（Update）**：`UpdateItem(const FString& Id, UPARAM(ref) const FMeetVRDanmakuItem& NewItem) → bool` — Id 必须匹配，其余字段替换，保留原 Id。
- **查（Read）**：`GetById(const FString& Id, bool& bFound) → FMeetVRDanmakuItem`、`Query(const FMeetVRDanmakuQuery& Filter) → TArray<FMeetVRDanmakuItem>`、`GetAll() → TArray<FMeetVRDanmakuItem>`、`Count() → int32`、`CountByQuery(const FMeetVRDanmakuQuery&) → int32`、`HasData() → bool`。
- **取下一条（触发消费）**：
  - `GetNext(const FMeetVRDanmakuQuery& Filter, bool& bWasEmpty) → FMeetVRDanmakuItem` — 筛选结果按 `Timestamp` 升序 + per-Query 游标循环；`bCyclePool=true` 循环复用游标，否则取出后从库移除；空置 `bWasEmpty=true` 返回空 Item。
  - 便捷重载 `GetNextBySpaceSection(const FString& SpaceId, const FString& SectionId, bool& bWasEmpty)`。
  - 内部 `TMap<uint32, int32> QueryCursor` 按 Query 哈希缓存游标。
- **HTTP 下载**：
  - `RequestDanmakuBatch(const FString& URL)` — GET → `ParseJsonResponse` → 按 `bClearBeforeBatch` 决定是否 `Clear()` → `AddBatch(...)` → 广播 `OnDownloadSucceeded(Count)`。请求去重（同 URL 在途忽略）。
  - `RequestDanmakuBatchFromDefaultURL()` — 用 `DefaultServerURL` 调上面。
  - `IsRequesting() const`。
- 委托：`OnDownloadSucceeded(int32 ItemCount)`、`OnDownloadFailed(const FString& Reason)`、`OnDatabaseChanged`（任何增删改后广播，便于 BP UI/监听刷新）。
- 私有：`TWeakPtr<IHttpRequest> ActiveRequest`、`FString ActiveRequestURL`、`ParseJsonResponse(const FString&)` 解析数组→`TArray<FMeetVRDanmakuItem>`、`ParseColorString`（`#RRGGBBAA`）、`ParseTypeString`、`ParseVectorObject`、`ParseRotatorObject`、`MatchesQuery`、`GenerateId`、`RebuildIndex`、`HashQuery`。
- HTTP 实现要点（仿 `MVLHttpBackendService`）：`FHttpModule::Get().CreateRequest()`，`SetURL/SetVerb("GET")`，`OnProcessRequestComplete()` 绑定 lambda，响应 `ResponseCode==200` 时 `GetContentAsString()` 交 `ParseJsonResponse`（`TJsonReaderFactory<>` + `FJsonSerializer`，依赖 `Json`/`JsonUtilities`）。

> 设计说明：数据库直接做成 `UGameInstanceSubsystem`（而非独立 UObject 由他类持有），享受单例生命周期与 `GetSubsystem<UMeetVRDanmakuDatabaseSubsystem>()` 直接访问；HTTP 下载与 CRUD 同处一个数据层入口，BP 一处拿到即可增删改查 + 下载，减少跨类引用。

### 5. 飞行载体 Actor（运动数据层）

**`Public/Actors/MeetVRDanmakuActor.h`** + **`Private/Actors/MeetVRDanmakuActor.cpp`** — `AActor`，`Blueprintable`，非复制（弹幕为本地表现）：

- 初始化：`Initialize(const FMeetVRDanmakuSpawnData& InData)`（BP 不可直接调，由组件 Spawn 后调用）。
- Tick：先扣减 `DelayTime`（>0 期间不移动、`GetAlpha` 返回0、不广播可见）；`DelayTime` 归零后开始 `AddActorWorldOffset(Velocity * DeltaTime)`，累计 `Elapsed`，到 `Lifetime` 触发销毁。
- 暴露（`BlueprintPure`）供 BP Widget 读取：`GetItem()`、`GetContent()`、`GetSpeaker()`、`GetColor()`、`GetFontSize()`、`GetType()`、`GetNormalizedProgress()`（0..1，相对 `Lifetime`）、`GetAlpha()`（延迟期=0；飞行期前后各 `FadeRatio` 段线性淡入淡出）。
- 配置：`float FadeRatio=0.15`、`bool bApplyGravity=false`、`float GravityScale`（可选，默认关，保持"简易"）。
- 委托：`OnDanmakuExpired`（生命周期结束广播，便于 BP 清理 Widget）。
- 默认构造无视觉组件（纯数据载体）；BP 子类附加 `UWidgetComponent` 渲染文字。

### 6. 触发组件（触发层）

**`Public/Components/MeetVRDanmakuSpawnComponent.h`** + **`Private/Components/MeetVRDanmakuSpawnComponent.cpp`** — `UActorComponent`，`Blueprintable, BlueprintType, ClassGroup=(MeetVR), meta=(BlueprintSpawnableComponent)`：

- 配置（`EditAnywhere`）：
  - `FString SpaceId`（场景/空间标识，触发时传给子系统筛选；空=通配）
  - `FString SectionId`（段落/触发点标识，触发时传给子系统筛选；空=通配）
  - `EMeetVRDanmakuFlightMode FlightMode=Upward`
  - `float FlightSpeed=150.0`（cm/s，Item 的 FlightSpeed>0 时覆盖此值）
  - `float Lifetime=3.0`（Item 的 Lifetime>0 时覆盖此值）
  - `float DefaultFontSize=1.0`（Item 的 FontSize>0 时覆盖此值）
  - `FVector SpawnOffset=FVector(0,0,50)`（相对 Owner）
  - `FVector CustomDirection=FVector(0,0,1)`（仅 `CustomDirection` 模式用）
  - `TSubclassOf<AMeetVRDanmakuActor> ActorClass`（默认 `AMeetVRDanmakuActor`，允许 BP 子类化）
  - `bool bAutoSpawnOnTrigger=true`（true=组件直接 Spawn Actor；false=仅广播数据让 BP 自行处理）
- BP 可调用函数：
  - `TriggerDanmaku()` — 用组件 `SpaceId`/`SectionId` 从子系统 `GetNextBySpaceSection` 取筛选后的下一条，合并默认值（lifetime/flightspeed/fontsize）→ 计算 spawn 数据 → 按 `bAutoSpawnOnTrigger` Spawn Actor 或仅广播。库空时广播 `OnDanmakuTriggered(SpawnData=空, null)` 便于 BP 处理无数据。
  - `TriggerDanmakuWithFilter(const FString& SpaceId, const FString& SectionId)` — 显式覆盖筛选的触发。
  - `TriggerDanmakuWithItem(const FMeetVRDanmakuItem& Item)` — 用指定项触发（不走池）。
  - `TriggerDanmakuAtLocation(FVector WorldLocation, const FMeetVRDanmakuItem& Item)` — 指定生成位置触发。
- `BlueprintPure`：`GetDatabaseSubsystem()`（获取 `UMeetVRDanmakuDatabaseSubsystem`，便于 BP 主动调 `RequestDanmakuBatch` 或直接 CRUD）。
- 委托：`OnDanmakuTriggered(const FMeetVRDanmakuSpawnData& SpawnData, AMeetVRDanmakuActor* SpawnedActor)`（BP 监听可做额外逻辑；SpawnedActor 可能为 null 当 `bAutoSpawnOnTrigger=false` 或池空）。
- 私有：`FVector ComputeDefaultSpawnLocation()`（基于 Owner + SpawnOffset）、`FRotator ComputeDefaultSpawnRotation()`（按 FlightMode 朝上/朝外）、`FVector ComputeVelocity(FVector SpawnLoc, float EffectiveSpeed)`（按 `FlightMode` 算方向，`OutwardFromCenter` 用 Owner 位置为参考做径向+向上混合）、`FMeetVRDanmakuSpawnData BuildSpawnData(const FMeetVRDanmakuItem& Item)`（合并规则：Lifetime/FlightSpeed/FontSize 在 Item 值>0 时覆盖组件默认；**Item.bHasLocation=true 时用 Item.Location/Rotation 作为 SpawnLocation/SpawnRotation，否则用 ComputeDefault***；DelayTime 直取 Item.DelayTime）。BeginPlay 不自动下载（由 BP 决定何时调 `RequestDanmakuBatch`），仅缓存子系统引用。

### 7. 配置文件

**`Config/DefaultMeetVRDanmaku.ini`** — 仿 `DefaultMeetVRSubtitle.ini`：
```
[/Script/Engine.GameplayTags]
+GameplayTagList=(Tag="Danmaku.System",DevComment="弹幕系统标签")

[/Script/MeetVRDanmaku.MeetVRDanmakuDatabaseSubsystem]
DefaultServerURL="http://localhost:8080/danmaku"
bCyclePool=True
bClearBeforeBatch=False
```

## 数据流（端到端）

1. BP 调 `SpawnComponent->GetDatabaseSubsystem()->RequestDanmakuBatchFromDefaultURL()`。
2. 子系统 HTTP GET → 解析 JSON（含 spaceid/sectionid/type/color/location/rotation 等全字段）→ 按 `bClearBeforeBatch` 决定是否先 `Clear()` → `AddBatch(...)` → 广播 `OnDownloadSucceeded` + `OnDatabaseChanged`。
3. BP 调 `SpawnComponent->TriggerDanmaku()`。
4. 组件用自身 `SpaceId`/`SectionId` 调子系统 `GetNextBySpaceSection` → 内部按 `Timestamp` 升序取下一条 → `BuildSpawnData` 合并默认值（Lifetime/FlightSpeed/FontSize 的 0 覆盖规则；`bHasLocation=true` 用 Item 方位）→ `ComputeVelocity` → 组装 `FMeetVRDanmakuSpawnData`。
5. 组件 Spawn `ActorClass`（默认 `AMeetVRDanmakuActor`）→ 调 `Initialize(SpawnData)` → 广播 `OnDanmakuTriggered(SpawnData, Actor)`；库空则 `bWasEmpty`、广播空数据。
6. Actor Tick：先扣 `DelayTime`（延迟期 alpha=0），归零后推进位置/生命周期；BP 子类的 `UWidgetComponent` 读取 `GetContent/GetColor/GetFontSize/GetAlpha` 渲染飞行字幕。
7. 到 Lifetime → Actor 广播 `OnDanmakuExpired` 并销毁。

**运行时 CRUD 旁路**：BP 也可直接 `GetSubsystem<UMeetVRDanmakuDatabaseSubsystem>()`（或经组件 `GetDatabaseSubsystem()`）调 `AddItem/RemoveById/UpdateItem/Query/GetAll` 任意增删改查，改动经 `OnDatabaseChanged` 广播刷新。

## 假设与决策

- **独立性**：仅依赖 UE 内置模块（Core/CoreUObject/Engine/GameplayTags/HTTP/Json/JsonUtilities），不依赖 MeetVRSubtitle/VaRest/MpyApiClient/MeetVRTemplate，满足"独立插件无依赖"。
- **非复制**：弹幕为本地表现型，不做网络同步（简易定位）；如未来需多人同步，另议。
- **视觉留 BP**：C++ 只提供运动载体与数据，Widget/材质/字体均由 BP 子类实现，符合"数据层优先"。
- **下载时机**：组件不自动下载，由 BP 显式调用，便于测试/调试与失败重试控制。
- **错误处理边界**：仅在系统边界（HTTP 响应、JSON 解析）做校验；内部数据流通透传。
- **字段决策（来自用户）**：
  - `spaceid`/`sectionid`：**用于触发时查询筛选**（非仅存储），空字符串=通配；子系统按 `SpaceId`+`SectionId` 过滤，支持多场景/多触发点共用一库。
  - `delaytime`：字段保留延迟语义，**当前默认0=触发即显示**；Actor 已支持非0延迟（延迟期不移动/alpha=0），未来按节奏配置即可，无需改结构。
  - `type`/`color`：枚举与 `#RRGGBBAA` 颜色解析在数据库子系统完成，存入 Item 供 BP 直接读取，不在渲染层二次解析。
  - `lifetime`/`flightspeed`/`fontsize`：Item 值>0 时覆盖组件默认（per-item 可调），=0 用组件默认，兼顾逐条配置与全局默认。
  - `location`/`rotation`/`haslocation`：`haslocation=true` 时用 Item 自带世界坐标方位覆盖组件默认生成位置/朝向；`false` 时由组件基于目标点 + SpawnOffset + FlightMode 计算。这样数据既可逐条指定绝对生成点，也可走"目标点触发"的相对模式。
- **内存数据库（来自用户）**：
  - 数据库直接做成 `UGameInstanceSubsystem`（`UMeetVRDanmakuDatabaseSubsystem`），单例生命周期，HTTP 下载与 CRUD 同处一个数据层入口；BP 经 `GetSubsystem<>()` 一处拿到即可增删改查 + 下载，无需跨类持有。
  - 纯内存、不持久化（重启清空，重新下载填充）；如未来需存档另议。
  - 索引：`IdIndex`（Id→下标）保证增删改查 O(1)/O(n) 级；`GetNext` 按 `Timestamp` 升序 + per-Query 游标循环。
  - CRUD 全部 `BlueprintCallable` 暴露，BP 可运行时直接增删改查 + 自定义 Query，不依赖 HTTP。
- **不做的事**：不实现碰撞触发 Actor、不做池预取调度、不做多语言/样式配置、不做数据持久化/存档（均留 BP/后续）。
- **文件名前缀**：所有文件 `MeetVRDanmaku` 前缀，类前缀 `MeetVR`，符合用户约定。

## 验证步骤

1. **编译**：启用插件后生成项目，确认 `MeetVRDanmaku` 模块无编译错误，依赖模块齐全。
2. **数据层单元验证**（BP 测试关卡）：
   - 放置一个 Actor（如空 Actor）挂 `MeetVRDanmakuSpawnComponent`，设置 `SpaceId`/`SectionId`。
   - BP 在 BeginPlay 调 `RequestDanmakuBatchFromDefaultURL()`，监听 `OnDownloadSucceeded` 打印 `ItemCount`，验证库大小与服务器返回一致；`OnDatabaseChanged` 也应广播。
   - 模拟一个本地 HTTP 服务器（如 `python -m http.server` 提供 JSON）验证解析，覆盖 `type`/`color`/`spaceid`/`sectionid`/`location`/`rotation`/`haslocation` 各字段。
   - 调 `TriggerDanmaku()`，验证 `OnDanmakuTriggered` 广播且 `SpawnedActor` 非空、`GetContent()`/`GetColor()`/`GetSpeaker()` 与 JSON 一致。
   - 设置不匹配的 `SpaceId`/`SectionId` 调 `TriggerDanmaku()`，验证 `bWasEmpty`、广播空数据不 Spawn。
3. **运动与方位验证**：Spawn 后观察 Actor 位置按 `FlightMode` 飞行；设某条 `haslocation=true` 且给绝对坐标，验证该条用数据方位生成而非组件 SpawnOffset；设 `DelayTime>0` 验证延迟期 alpha=0 且不移动，归零后开始飞行；到 `Lifetime` 自动销毁并触发 `OnDanmakuExpired`。
4. **CRUD 验证**：BP 调 `GetSubsystem<UMeetVRDanmakuDatabaseSubsystem>()`（或组件 `GetDatabaseSubsystem()`）：
   - `AddItem`（空 Id）→ 验证返回自动生成 Id 且 `Count+1`；`AddItem`（已存在 Id）→ 覆盖更新。
   - `GetById`/`Query`（按 SpaceId/SectionId/Type）→ 返回正确子集。
   - `UpdateItem(Id, NewItem)` → 字段已替换、Id 不变。
   - `RemoveById`/`RemoveBySpaceId`/`RemoveByQuery` → 删除数正确、`IdIndex` 一致（再次 `GetById` 返回 `bFound=false`）。
   - 每次增删改后 `OnDatabaseChanged` 广播。
5. **边界**：服务器不可达 → `OnDownloadFailed` 触发；`TriggerDanmaku` 在库空时返回空 Item 且不 Spawn；`bClearBeforeBatch=true` 时重新下载后旧数据清空。
6. **BP 视觉对接**：开发者创建 `BP_DanmakuActor`（继承 `AMeetVRDanmakuActor`，加 `WidgetComponent` 读 `GetContent/GetColor/GetFontSize/GetAlpha`），将 `ActorClass` 设为该 BP 类，验证飞行字幕可见且颜色/字号正确。
