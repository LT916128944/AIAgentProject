# SuckerStatistics 统计系统问题与修改建议

> 本文档基于对 `Plugins/SuckerFramework/Source/SuckerFramework/Statistics/` 的静态代码审查,列出**当前使用会遇到的问题**与**需要修改的地方**,按严重程度分级。每条均标注真实代码位置,可直接对照核实。

---

## 一、严重问题(会导致功能错误 / 数据丢失,使用前必须修)

### 问题 1:CalcDamage 查询标签硬编码,道具伤害永远为 0
- **位置**:[SuckerStatisticsComponent.cpp#L86-L103](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L86-L103)
- **现象**:`CalcDamage` 第 91 行**硬编码**查询 `TAG_Stat_Value_Damage_Missile`:
  ```cpp
  TArray<FSuckerStatisticsRecord> TempRecords = Subsystem->GetRecordsByTag(SuckerStatisticsTags::TAG_Stat_Value_Damage_Missile);
  ```
  而 `GetRecordsByTag` 用 `Verb.MatchesTag(InTag)` 过滤。道具伤害记录的 Verb 是 `Stat.Value.Damage.Item`,它**不是** `Stat.Value.Damage.Missile` 的子标签,所以永远不会被查出来。
- **后果**:第 10-17 行两次调用(道具伤害 Player2Monster / Player2Weakness)传入 `TAG_Stat_Value_Damage_Item`,但在 Missile 记录集里按 `Verb == Item` 过滤 → **恒为 0**。即使用现在,道具伤害统计永远拿不到。
- **修改**:查询标签应取更通用的父标签 `TAG_Stat_Value_Damage`(或新增 `Stat.Value`),而非硬编码 Missile:
  ```cpp
  TArray<FSuckerStatisticsRecord> TempRecords = Subsystem->GetRecordsByTag(SuckerStatisticsTags::TAG_Stat_Value_Damage);
  ```
  这样 Item/Missile.Normal/Missile.Super 等所有伤害子记录都会被召回,再用第三参 `MissileType` 精确过滤。

### 问题 2:GetAllStatisticResults 键重复,道具伤害结果被覆盖
- **位置**:[SuckerStatisticsComponent.cpp#L14-L31](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L14-L31)
- **现象**:第 15 行与第 29 行使用了**相同**的键 `TAG_Stat_Calc_Damage_Item_Player2Weakness`:
  ```cpp
  // 第15行:正确——道具伤害 Player2Weakness
  Result.Add(TAG_Stat_Calc_Damage_Item_Player2Weakness, CalcDamage("Player","Weakness", TAG_Stat_Value_Damage_Item));
  // 第29行:错误——导弹 Normal 伤害却写进了道具伤害的键
  Result.Add(TAG_Stat_Calc_Damage_Item_Player2Weakness, CalcDamage("Player","Weakness", TAG_Stat_Value_Damage_Missile_Normal));
  ```
- **后果**:`TMap::Add` 对已存在的键会覆盖。第 29 行用 Missile_Normal 数据覆盖了第 15 行的 Item 数据,且 `TAG_Stat_Calc_Damage_Missile_Normal_Player2Weakness` 这个本应存在的键**完全没被产出**。
- **修改**:第 29 行键应为 `TAG_Stat_Calc_Damage_Missile_Normal_Player2Weakness`:
  ```cpp
  Result.Add(SuckerStatisticsTags::TAG_Stat_Calc_Damage_Missile_Normal_Player2Weakness,
      CalcDamage(TEXT("Player"), TEXT("Weakness"), SuckerStatisticsTags::TAG_Stat_Value_Damage_Missile_Normal));
  ```

### 问题 3:UnregisterListener 未清理 Handles 表,导致无法重新注册
- **位置**:[SuckerStatisticsSubsystem.cpp#L89-L98](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsSubsystem.cpp#L89-L98)
- **现象**:`UnregisterListener` 从消息总线注销了监听,但**没有** `Handles.Remove(Channel)`:
  ```cpp
  bool USuckerStatisticsSubsystem::UnregisterListener(FGameplayTag Channel)
  {
      if (FGameplayMessageListenerHandle* FoundHandle = Handles.Find(Channel))
      {
          MessageSystem.UnregisterListener(*FoundHandle);
          return true;   // ← 缺少 Handles.Remove(Channel)
      }
      return false;
  }
  ```
- **后果**:`RegisterListener` 第 82 行用 `Handles.Find(Channel)` 防重,残留的旧句柄会让后续注册**永远返回 false**。一旦任何 Component 触发 EndPlay(注销),即便 Subsystem 仍在运行,也无法再恢复监听 → 数据收集永久停止。
- **修改**:在 `UnregisterListener` 中注销后补 `Handles.Remove(Channel)`:
  ```cpp
  MessageSystem.UnregisterListener(*FoundHandle);
  Handles.Remove(Channel);
  return true;
  ```

---

## 二、设计缺陷(影响可用性 / 健壮性,强烈建议改)

### 问题 4:监听器生命周期由 Component 掌管,Subsystem 自身不注册
- **位置**:[SuckerStatisticsSubsystem.cpp#L8-L26](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsSubsystem.cpp#L8-L26)(Initialize / OnWorldBeginPlay 均为空)、[SuckerStatisticsComponent.cpp#L63-L83](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L63-L83)
- **现象**:数据收集的监听注册/注销完全由 `USuckerStatisticsComponent` 的 BeginPlay/EndPlay 触发,Subsystem 自己的 `Initialize`/`OnWorldBeginPlay` 没有注册任何监听。
- **后果**:
  1. 若场景中**没有**挂载 StatisticsComponent 的 Actor,Subsystem 收不到任何消息 → Records 永远为空。
  2. Component 的 Actor 被销毁(EndPlay)后,监听被注销,但 Subsystem 仍存活 → 之后发生的所有事件**全部丢失**。
  3. 多个 Component 并存时,只有最先 BeginPlay 的那个能注册成功(`Handles.Find` 防重),但任一 Component EndPlay 都会把监听注销,影响全局。
- **修改建议**:把监听注册收归 Subsystem 自身。在 `Initialize`(或 `OnWorldBeginPlay`)中注册 `Stat.Count / Stat.Time / Stat.Value` 三个频道,在 `Deinitialize` 中注销。Component 只负责查询,不再负责注册/注销监听。这样数据收集与 Component 生命周期解耦,只要 World 存在就持续收集。

### 问题 5:TAG_Stat_Value 与 TAG_Stat_Value_Damage 指向同一字符串
- **位置**:[SuckerStatisticsTags.cpp#L8-L9](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsTags.cpp#L8-L9)
- **现象**:
  ```cpp
  UE_DEFINE_GAMEPLAY_TAG(TAG_Stat_Value,        "Stat.Value.Damage");  // 期望是 Stat.Value
  UE_DEFINE_GAMEPLAY_TAG(TAG_Stat_Value_Damage, "Stat.Value.Damage");  // 实际也是 Stat.Value.Damage
  ```
- **后果**:Component 第 70 行注册 `TAG_Stat_Value`,实际注册的是 `Stat.Value.Damage` 而非 `Stat.Value`。当前所有 Value 都是 Damage,所以「巧合」能工作;但命名有误导性,且若未来新增非 Damage 的 Value 类统计(如 `Stat.Value.Heal`),将收不到。
- **修改**:`TAG_Stat_Value` 应定义为 `"Stat.Value"`,与 `TAG_Stat_Value_Damage`(`"Stat.Value.Damage"`)区分层级。

### 问题 6:SumStatTags / TimeStatTags 声明但未使用
- **位置**:[SuckerStatisticsComponent.h#L29-L36](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsComponent.h#L29-L36)
- **现象**:头文件声明了 `CountStatTags`、`SumStatTags`、`TimeStatTags` 三个容器,但实现里 `CalcAllCountStatTags` 只遍历了 `CountStatTags`,另两个完全没被读取。
- **后果**:编辑器里能配置 SumStatTags / TimeStatTags,但配置了也不生效,产生「配了没用」的困惑。Time 类统计(如战斗开始/结束时间)当前无法通过组件聚合查询。
- **修改**:要么在 `GetAllStatisticResults` 中补充对 `SumStatTags`(求和)、`TimeStatTags`(取首/末时间戳)的处理;要么若暂不需要则删除这两个未用成员,避免误导。

---

## 三、一般问题(影响可维护性 / 性能,建议改)

### 问题 7:CalcDamage 用字符串 Contains 匹配 Instigator/Target,脆弱
- **位置**:[SuckerStatisticsComponent.cpp#L94-L95](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L94-L95)
- **现象**:`ItRecord.Message.Target.Contains(InTargetKey)` 用 `FString::Contains` 做子串匹配。Instigator/Target 存的是 `UObject::GetName()`(如 `BP_PlayerCharacter_C_42`、`BP_MonsterBoss_C_0`)。
- **后果**:子串匹配容易误命中——例如名为 `PlayerProjectile` 的 Actor 会匹配 `Player`;且 Actor 名带实例后缀 `_42`,匹配规则隐式依赖命名约定,换地图/换蓝图就可能失效。
- **修改建议**:落库时改存更稳定的标识(如 GameplayTag 或角色类别标签),或用 `==` 精确匹配前缀,而非 `Contains`。

### 问题 8:ExportRecords 无返回值,无法感知导出失败
- **位置**:[SuckerStatisticsSubsystem.h#L74-L75](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsSubsystem.h#L74-L75)、[SuckerStatisticsSubsystem.cpp#L54-L73](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsSubsystem.cpp#L54-L73)
- **现象**:`ExportRecords()` 声明为 `void`,内部 `SaveJsonToFile` 返回 bool 但被丢弃。
- **后果**:蓝图层调用导出后无法知道是否成功(磁盘满/路径无权限时静默失败)。
- **修改**:改为 `bool ExportRecords()` 并返回 `SaveJsonToFile` 的结果。

### 问题 9:Records 无上限增长,查询为 O(n) 全表扫描
- **位置**:[SuckerStatisticsSubsystem.h#L93-L94](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Public/Statistics/SuckerStatisticsSubsystem.h#L93-L94)
- **现象**:`TArray<FSuckerStatisticsRecord> Records` 只增不减(除非主动 ClearRecords);`GetTotalResultByTag` / `GetRecordsByTag` 每次都遍历全部记录。
- **后果**:长局对战中记录数持续增长,`GetAllStatisticResults` 对每个 CountStatTag 调一次 `GetTotalResultByTag` → O(n × m),频繁查询会卡顿。
- **修改建议**:可选——按频道/标签预聚合缓存(收到消息时增量更新汇总值),或对 Records 设上限并滚动淘汰最旧记录。

### 问题 10:CalcDamage 被优化关闭包裹,暗示存在编译/优化问题
- **位置**:[SuckerStatisticsComponent.cpp#L85](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L85)、[L104](file:///f:/LiaoBo_VR/Plugins/SuckerFramework/Source/SuckerFramework/Private/Statistics/SuckerStatisticsComponent.cpp#L104)
- **现象**:`UE_DISABLE_OPTIMIZATION` / `UE_ENABLE_OPTIMIZATION` 包裹了 `CalcDamage`,说明此处曾因优化导致问题而临时关闭优化。
- **后果**:该函数在 Shipping 也会关闭优化,影响性能;且根因未定位,可能随编译器版本复现。
- **修改建议**:排查为何需要关优化(常见于浮点累加被 SIMD 重排导致精度问题),修复后移除这两个宏。

---

## 四、问题汇总速查表

| # | 严重度 | 问题 | 位置 | 使用现状影响 |
|---|--------|------|------|--------------|
| 1 | 严重 | CalcDamage 查询标签硬编码为 Missile | Component.cpp:91 | 道具伤害统计恒为 0 |
| 2 | 严重 | GetAllStatisticResults 键重复覆盖 | Component.cpp:15,29 | Missile_Normal→Weakness 结果丢失,Item 数据被错值覆盖 |
| 3 | 严重 | UnregisterListener 未清 Handles 表 | Subsystem.cpp:89-98 | 注销后无法重注册,数据收集永久停止 |
| 4 | 设计缺陷 | 监听生命周期由 Component 掌管 | Subsystem.cpp:8-26 | 无 Component 或 Component 销毁后,事件全部丢失 |
| 5 | 设计缺陷 | TAG_Stat_Value 字符串与 Damage 相同 | Tags.cpp:8-9 | 命名误导,扩展非 Damage 数值会漏收 |
| 6 | 设计缺陷 | SumStatTags/TimeStatTags 未使用 | Component.h:29-36 | 配置不生效,Time 类统计无法聚合 |
| 7 | 一般 | Instigator/Target 用 Contains 子串匹配 | Component.cpp:94-95 | 命名变动易误匹配或漏匹配 |
| 8 | 一般 | ExportRecords 无返回值 | Subsystem.h:74 | 导出失败无法感知 |
| 9 | 一般 | Records 无上限 + O(n) 查询 | Subsystem.h:93 | 长局查询性能下降 |
| 10 | 一般 | CalcDamage 被关优化包裹 | Component.cpp:85,104 | 性能损失,根因未修 |

---

## 五、若现在直接使用会出现的实际现象

1. **道具伤害统计永远为 0**:无论玩家用道具打了多少伤害,`TAG_Stat_Calc_Damage_Item_Player2Monster / Player2Weakness` 查出来都是 0(问题 1)。
2. **导弹 Normal→Weakness 伤害丢失**:这个交叉统计在结果 Map 里根本没有独立键,被道具伤害键错误覆盖(问题 2)。
3. **Component 一旦销毁,统计立刻停摆**:挂载 StatisticsComponent 的 Actor(如玩家)死亡/切换时 EndPlay 触发注销,之后整局不再收集任何数据,且无法恢复(问题 3 + 4)。
4. **无 Component 则无数据**:若关卡里没有任何 Actor 挂 StatisticsComponent,Subsystem 不会注册任何监听,Records 恒空(问题 4)。
5. **导出可能静默失败**:调 ExportRecords 后不知道文件是否真的写成功(问题 8)。
6. **长局性能退化**:战斗越久 Records 越大,每次查询全表扫描(问题 9)。

---

## 六、建议修改优先级(实施顺序)

1. **先修问题 1、2、3**(严重 bug,5 分钟内可改完):改查询标签、改重复键、补 Handles.Remove。修完后道具/导弹伤害统计才能正确产出。
2. **再修问题 4**(架构调整):把监听注册移到 Subsystem.Initialize,Component 只查询不注册。这是保证「数据不丢」的关键。
3. **接着修问题 5、6**(语义对齐):Tag 字符串纠正、未用成员处理或补实现。
4. **最后处理问题 7-10**(健壮性/性能):按实际使用反馈逐步改进。

> 说明:以上均为基于代码静态审查的结论。问题 1/2/3 可通过「构造对应 VerbMessage 广播 → 调 GetAllStatisticResults」在编辑器内快速验证修复效果。
