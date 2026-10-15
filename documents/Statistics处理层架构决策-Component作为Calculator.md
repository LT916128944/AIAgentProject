# Statistics 处理层架构决策:Component 作为 Calculator

> 本文档确认统计系统的处理层架构,并给出让 Component 成为合格「专用 calculator」的最小化调整方案。

## 一、架构决策

**Subsystem(存储 + 收集) + Component(处理 + 查询)** 的分工保留不变:

| 层 | 对象 | 职责 | 生命周期 |
|----|------|------|----------|
| 存储层 | `USuckerStatisticsSubsystem`(GameInstanceSubsystem) | 收集消息落库、存储 Records、基础查询(GetTotalResultByTag/GetRecordsByTag)、导出 | 随 GameInstance,跨关卡持久 |
| 处理层 | `USuckerStatisticsComponent`(ActorComponent) | 持有报表配置(CountStatTags/SumStatTags/TimeStatTags)、聚合查询(GetAllStatisticResults)、派生计算(CalcDamage) | 随挂载的 Actor |

### 为什么 Component 作为 calculator 合理(在本项目语境下)

1. **报表配置可 per-instance 差异化**:不同 Component 实例可配置不同的 CountStatTags,产出不同维度的报表。这是 Component 相比 Subsystem 单例的优势——Subsystem 是单例,只能有一份全局配置。
2. **蓝图调用入口友好**:Component 挂在 PlayerController/GameState 上,蓝图通过 `GetComponent` 即可调用,符合 UE 使用习惯。
3. **职责已经分离**:存储与收集在 Subsystem,处理在 Component,符合「单一职责」。

### 使用时必须遵守的约束(否则会踩坑)

- **Component 必须挂在持久 Actor 上**(如 PlayerController、GameMode、GameState),不能挂在会被频繁销毁的 Actor(如怪物)上——否则 Actor 销毁后查询入口消失,但 Subsystem 数据仍在。
- **每个报表定义用一个 Component 实例**:避免多个 Component 配置相同 CountStatTags 重复计算。
- **Component 计算的是全局数据**:CalcDamage 查的是 Subsystem 的全局 Records,不是 Component 所属 Actor 的个人数据。这是有意设计,不是 bug。

## 二、需要调整的地方(最小化)

当前 Component 作为 calculator 已基本可用,但有几处需调整以稳固其角色:

### 改动 1:CalcDamage 内 Instigator/Target 匹配改用精确前缀匹配
- **文件**:[SuckerStatisticsComponent.cpp#L105-L107](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L105-L107)
- **现状**:用 `FString::Contains` 子串匹配,`"PlayerProjectile"` 会误匹配 `"Player"`
- **改为**:用 `StartsWith` 做前缀匹配,配合固定前缀(`"BP_Player"`/`"BP_Monster"`/`"BP_Weakness"`),减少误命中
- **为什么**:calculator 的输出正确性依赖匹配精度,这是当前最容易出错的点

### 改动 2:移除 CalcDamage 的优化关闭宏
- **文件**:[SuckerStatisticsComponent.cpp#L96](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L96)、[L115](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L115)
- **现状**:`UE_DISABLE_OPTIMIZATION` / `UE_ENABLE_OPTIMIZATION` 包裹 CalcDamage
- **改为**:移除这两个宏。CalcDamage 只是简单的 float 累加 + 字符串比较,不存在需要关优化的复杂逻辑。之前的宏可能是历史遗留,移除后若 Shipping 出问题再针对性排查。
- **为什么**:calculator 作为处理层应保持性能;关优化宏是未定位根因的临时手段,不应长期保留

### 改动 3:为 Component 添加角色说明,明确使用约束
- **文件**:[SuckerStatisticsComponent.h](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsComponent.h)
- **现状**:类无任何注释,使用者不清楚应挂在哪、计算的是全局还是局部数据
- **改为**:在类声明上方添加简短注释,说明:① 这是统计计算器,查询 Subsystem 全局数据;② 应挂在持久 Actor 上;③ 配置(CountStatTags 等)定义该实例要计算哪些报表
- **为什么**:calculator 的正确使用依赖约束可知性,一行注释即可避免误用

## 三、不改动的地方(明确排除)

- **不把处理逻辑移入 Subsystem**:用户已决定保留 Component 作为 calculator,Subsystem 维持纯存储+收集
- **不引入 Calculator UObject**:用户选择沿用 Component,不新增独立对象
- **不把 CalcDamage 改为数据驱动配置**:当前硬编码的 Player2Monster 等组合是固定的报表定义,数量有限,数据驱动会过度设计;若未来报表组合激增再考虑
- **不改 CountStatTags 等 UPROPERTY 的 EditDefaultsOnly**:per-instance 配置是 Component 作为 calculator 的核心价值,保留

## 四、验证

1. 改动 1 后:在编辑器内广播一条 Instigator 名含 "PlayerProjectile" 的伤害消息,确认不会被 `Player` 前缀匹配误计入玩家伤害
2. 改动 2 后:Development/Shipping 编译 CalcDamage 无优化警告,伤害聚合结果与改前一致
3. 改动 3 后:头文件注释可见,无编译影响
