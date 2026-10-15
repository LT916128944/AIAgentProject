# SuckerStatistics 统计系统详解

> 本文档是对项目中 **Statistics(统计)系统** 的只读分析说明,不涉及任何代码改动。

## 一、系统定位

Statistics 系统位于 `Plugins/SuckerFramework` 插件中,负责**收集、存储、查询、导出**游戏过程中的战斗/事件统计数据。它采用「事件驱动 + 消息总线」的架构:游戏各处通过 `UGameplayMessageSubsystem` 广播 `FSuckerVerbMessage`,统计子系统监听这些消息并落库为 `FSuckerStatisticsRecord`,再由统计组件按需聚合查询。

核心解决的问题是:谁(Instigator)对谁(Target)做了什么(Verb)、数值多大(Magnitude)、何时发生(Time),并支持按 GameplayTag 维度汇总与导出。

## 二、文件结构

```
Plugins/SuckerFramework/Source/SuckerFramework/
├── Public/Statistics/
│   ├── SuckerStatisticsSubsystem.h     // 统计子系统(数据中枢)
│   ├── SuckerStatisticsComponent.h     // 统计组件(查询/聚合)
│   └── SuckerStatisticsTags.h          // 统计标签定义
├── Private/Statistics/
│   ├── SuckerStatisticsSubsystem.cpp
│   ├── SuckerStatisticsComponent.cpp
│   └── SuckerStatisticsTags.cpp
└── Public/Message/SuckerVerbMessage.h  // 广播消息结构(数据来源)
```

## 三、核心数据结构

### 1. FSuckerVerbMessage(广播消息 — 数据来源)
[SuckerVerbMessage.h](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Message/SuckerVerbMessage.h)

```cpp
struct FSuckerVerbMessage
{
    FGameplayTag Verb;              // 动作类型(如 Stat.Value.Damage.Missile.Normal)
    TObjectPtr<UObject> Instigator; // 施加者(对象指针)
    TObjectPtr<UObject> Target;     // 承受者(对象指针)
    FGameplayTagContainer InstigatorTags; // 施加者标签
    FGameplayTagContainer TargetTags;     // 承受者标签
    FGameplayTagContainer ContextTags;    // 上下文标签
    double Magnitude = 1.0;         // 数值(伤害量/计数)
};
```
这是通用「主谓宾」消息结构,由游戏逻辑(如 AttributeSet)广播到消息总线。

### 2. FSuckerStatisticsMessage(存储消息 — 落库格式)
[SuckerStatisticsSubsystem.h#L11-L33](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsSubsystem.h#L11-L33)

```cpp
struct FSuckerStatisticsMessage
{
    FGameplayTag Verb;
    FString Instigator;        // 注意:已从 UObject* 转为字符串名
    FString Target;            // 便于序列化与过滤
    FGameplayTagContainer ContextTags;
    double Magnitude = 1.0;

    static FSuckerStatisticsMessage ConvertMessage(FSuckerVerbMessage InVerbMessage);
};
```
**关键设计**:存储时把 `UObject*` 转成 `FString` 名字,避免悬空指针,同时便于 JSON 序列化和字符串过滤匹配。

### 3. FSuckerStatisticsRecord(统计记录条目)
[SuckerStatisticsSubsystem.h#L35-L49](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsSubsystem.h#L35-L49)

```cpp
struct FSuckerStatisticsRecord
{
    FGameplayTag StatTag;               // 接收频道(如 Stat.Value.Damage)
    FSuckerStatisticsMessage Message;   // 消息内容
    FDateTime Time;                     // 记录时间(UTC)
};
```
这是子系统 `Records` 数组中每一条记录的最终形态。

## 四、核心类详解

### 1. USuckerStatisticsSubsystem(数据中枢)
[SuckerStatisticsSubsystem.h](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsSubsystem.h) | [实现](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsSubsystem.cpp)

继承 `UWorldSubsystem`,是统计系统的存储与查询中心。

**数据成员:**
```cpp
TArray<FSuckerStatisticsRecord> Records;                          // 全部记录
TMap<FGameplayTag, FGameplayMessageListenerHandle> Handles;       // 频道→监听句柄
```

**核心 API:**

| 方法 | 职责 |
|------|------|
| `RegisterListener(Channel)` | 在消息总线上注册某频道的监听器 |
| `UnregisterListener(Channel)` | 注销监听(用 Handle 查表) |
| `OnNotificationMessage(...)` | **回调核心**:收到 VerbMessage → Convert → 建 Record(带时间戳)→ `Records.Add` |
| `GetTotalResultByTag(Tag)` | 遍历 Records,对 `Verb.MatchesTag(Tag)` 的记录累加 Magnitude |
| `GetRecordsByTag(Tag)` | 返回匹配标签的记录子集 |
| `ExportRecords()` | 把每条 Record 序列化为 JSON,保存到 `Saved/NewRecord_YYYYMMDD_HHMMSS.json` |
| `ClearRecords()` | 清空全部记录 |

**关键实现细节([SuckerStatisticsSubsystem.cpp#L100-L108](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsSubsystem.cpp#L100-L108)):**
```cpp
void USuckerStatisticsSubsystem::OnNotificationMessage(FGameplayTag Channel, const FSuckerVerbMessage& Notification)
{
    FSuckerStatisticsRecord TempRecord;
    TempRecord.StatTag = Channel;
    TempRecord.Message = FSuckerStatisticsMessage::ConvertMessage(Notification);
    TempRecord.Time = FDateTime::UtcNow();
    Records.Add(TempRecord);
}
```
`MatchesTag` 是层级匹配(父标签可匹配子标签),所以监听 `Stat.Value` 频道能收到 `Stat.Value.Damage.Missile.Normal` 等所有子标签消息。

### 2. USuckerStatisticsComponent(查询/聚合组件)
[SuckerStatisticsComponent.h](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsComponent.h) | [实现](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp)

继承 `UActorComponent`,挂在需要查询统计数据的 Actor 上(如玩家控制器/游戏状态)。

**配置成员:**
```cpp
FGameplayTagContainer CountStatTags; // 需统计计数的标签
FGameplayTagContainer SumStatTags;   // 需统计求和的标签
FGameplayTagContainer TimeStatTags;  // 需统计时间的标签
```

**生命周期([SuckerStatisticsComponent.cpp#L63-L83](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L63-L83)):**
- `BeginPlay`:向子系统注册三个频道监听:`Stat.Count`、`Stat.Time`、`Stat.Value`
- `EndPlay`:注销这三个监听

**聚合查询 `GetAllStatisticResults()`([SuckerStatisticsComponent.cpp#L4-L48](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L4-L48)):**
返回 `TMap<FGameplayTag, float>`,包含两类结果:
1. **计数类**(`CalcAllCountStatTags`):遍历 `CountStatTags`,对每个标签调用 `GetTotalResultByTag` 累加 Magnitude
2. **伤害计算类**(`CalcDamage`):按 Instigator/Target/Verb 三重过滤后累加 Magnitude,产出派生统计标签:
   - `Stat.Value.Damage.Item.Player2Monster` / `Player2Weakness`
   - `Stat.Value.Damage.Missile.Normal.Player2Monster` / `Player2Weakness`
   - `Stat.Value.Damage.Missile.Super.Player2Monster` / `Player2Weakness`
   - `Stat.Value.Damage.Missile.Normal/Super.Monster2Player`

**注意**:`GetAllStatisticResults()` 中存在一处疑似重复键:`TAG_Stat_Calc_Damage_Item_Player2Weakness` 被添加了两次(一次用 `Damage_Item`,一次用 `Damage_Missile_Normal`),后者会覆盖前者([SuckerStatisticsComponent.cpp#L14-L31](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L14-L31))。

### 3. SuckerStatisticsTags(标签体系)
[SuckerStatisticsTags.h](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsTags.h) | [实现](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsTags.cpp)

标签分三大类,均以 `Stat.` 为根:

**① Value(数值类)— 伤害值**
```
Stat.Value.Damage                  // 父标签
├── Stat.Value.Damage.Missile      // 导弹伤害
│   ├── .Normal / .Mirror / .Super
├── Stat.Value.Damage.Item         // 道具伤害
└── Stat.Value.Damage.*.Player2Monster / Player2Weakness / Monster2Player  // 派生计算结果
```

**② Count(计数类)— 事件次数**
```
Stat.Count                         // 父标签
├── Stat.Count.Missile(.Normal/.Mirror/.Super)   // 导弹发射数
├── Stat.Count.Item                               // 道具使用数
├── Stat.Count.Death.Player/Boss/Weakness         // 死亡数
├── Stat.Count.Hit.Boss/Boss_Mirror/Missile/Player // 命中数
├── Stat.Count.Weapon.Disable/Fire(.Missile/.Item) // 武器事件
└── Stat.Count.Monster.Teleport/SpeAttack/Rampage  // 怪物技能次数
```

**③ Time(时间类)— 关键时间点**
```
Stat.Time
├── Stat.Time.Game.Start / Game.End
└── Stat.Time.Fight.Start / Fight.End
```

## 五、数据流(端到端)

```
┌─────────────────────────────────────────────────────────────────┐
│  游戏事件源                                                       │
│  (SuckerAttributeSet 等)                                          │
│         │                                                         │
│         │ 构造 FSuckerVerbMessage                                  │
│         │ (Verb/Instigator/Target/Magnitude)                      │
│         ▼                                                         │
│  UGameplayMessageSubsystem.BroadcastMessage(频道, Message)        │
│         │                                                         │
│         │ 消息总线分发到该频道所有监听者                            │
│         ▼                                                         │
│  USuckerStatisticsSubsystem.OnNotificationMessage()               │
│         │                                                         │
│         │ ConvertMessage: UObject* → FString                      │
│         │ 生成 FSuckerStatisticsRecord(带 FDateTime)              │
│         ▼                                                         │
│  Records TArray 落库                                              │
│         │                                                         │
│         ├──────► GetTotalResultByTag / GetRecordsByTag (查询)     │
│         │                ▲                                        │
│         │                │ 查询/聚合                               │
│         │          USuckerStatisticsComponent                     │
│         │          (CalcAllCountStatTags / CalcDamage)            │
│         │                                                         │
│         └──────► ExportRecords() → JSON 文件(Saved 目录)         │
└─────────────────────────────────────────────────────────────────┘
```

### 数据来源实例:伤害广播
[SuckerAttributeSet.cpp#L149-L167](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/AbilitySystem/SuckerAttributeSet.cpp#L149-L167)

当 GAS 执行伤害 GameplayEffect 时:
1. 从 EffectContext 取 Causer 作为 Instigator,自身 Actor 作为 Target
2. 默认 Verb = `Stat.Value.Damage`,再用 InstigatorTags 过滤出更具体的子标签(如 `Stat.Value.Damage.Missile.Normal`)
3. `BroadcastMessage(Stat.Value.Damage, Message)` 广播

子系统监听了 `Stat.Value` 父频道,故能收到所有伤害子标签消息并落库。

## 六、设计特点总结

| 特性 | 说明 |
|------|------|
| **事件驱动解耦** | 数据源只管广播,不依赖统计系统;统计系统通过消息总线被动接收,松耦合 |
| **GameplayTag 层级匹配** | 监听父标签即可收到所有子标签消息,`MatchesTag` 实现层级查询汇总 |
| **UObject → FString 转换** | 落库时去对象化,避免悬空指针,支持序列化与字符串过滤 |
| **三层标签分类** | Value(数值)/ Count(计数)/ Time(时间),语义清晰 |
| **派生统计** | CalcDamage 在原始记录上做 Instigator×Target×Verb 三维过滤聚合,产出 Player2Monster 等交叉统计 |
| **JSON 导出** | FJsonObjectConverter 一行序列化,便于离线分析与回放对账 |
| **World 生命周期** | UWorldSubsystem 随关卡存活,Records 在关卡内累积;切关卡会重建 |

## 七、当前可观察的待改进点(仅记录,不在本次范围内修改)

1. **监听器注册重复**:Component 的 `BeginPlay` 注册 `Stat.Count/Time/Value`,但 Subsystem 的 `RegisterListener` 用 `Handles.Find(Channel)` 防重复;若多个 Component 同时存在,只有第一个能注册成功,后续 Component 查询时数据已在中枢但仍可能因未注册而漏收(取决于谁负责注册)。
2. **`GetAllStatisticResults` 键覆盖**:`TAG_Stat_Calc_Damage_Item_Player2Weakness` 被重复 `Result.Add`,第二次用的 Missile_Normal 数据会覆盖第一次的 Item 数据。
3. **`SumStatTags` / `TimeStatTags` 声明了但未使用**:Component 头部声明了三个标签容器,但实现只用了 `CountStatTags`,另两个当前为空跑。
4. **ExportRecords 无返回值**:导出成功与否无法在蓝图层感知(`SaveJsonToFile` 返回 bool 但 `ExportRecords` 为 void)。
5. **无持久化**:Records 仅存内存,切关卡/退出即丢失,需主动 `ExportRecords` 才能留存。

---

以上为 SuckerStatistics 系统的完整静态分析。该系统目前功能集中在「伤害数值记录 + 计数汇总 + JSON 导出」,架构清晰,采用消息总线解耦,符合项目「数据层优先、单职责」的工程约定。
