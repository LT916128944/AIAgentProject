# 外部 UE5 经验库（来源 UnrealEngine5-Skills-main，MIT，500+ star）

用户裁决（2026-09-21）：**无需本机验证，按有效采纳**；哪天实际对不上再回来改这两处即可。
用法：**禁止整文件读**，按 `## ` 标题 `rg` 窄查，例如
`rg -n -A 20 '^## PCG' .codex/skills/ue-low-token-skill/references/ext_ue5_knowledge.md`
本文件只提供流程与领域判据；**MCP 工具名/参数一律以 `ue_mcp_index.json` + `ue_mcp_verified.json` 为准**。

## 通用工作法

- 阶段契约：动手前把「输入/对象 → 核心步骤 → 参数 → 输出 → 验证方式」定齐，缺一项就不算可交付。
- 症状 → 定位 → 修复：每张失败表都按这三列用；一次只验一个假设，别把 instrument 与功能修复混在一步。
- 工具优先级：专有工具 > 一次脚本编排 > bridge；`execute_tool_script` 只在「无专有工具可做 + 无法拆解 + 范围最小可逆 + 已在回复说明」四条同时成立时用。
- 路由输出契约（跨域请求必给）：`primary_skill` / `secondary_skill` / `recommended_mcp_tools[]` / `route_confidence` / `route_reason`；歧义时只问一次「A 还是 B」。
- 最小复现模板：引擎版本 / 地图 / 相关资产类 / 网络模式 / 复现步骤 / 期望 / 实际 / 证据（日志片段、截图、状态转储）。
- 证据纪律：结论必须能指向一条日志、一个属性值或一次工具返回；没有证据就写「未验证」。

## 调试与验证

- 六步：复现并冻结上下文 → 采集信号（按分类/时间窗过滤日志）→ 校验资产与配置存在性 → 定位「第一处坏点」→ 假设按概率×验证成本排序、逐个低干扰验证 → 最小修复 + 回归范围。
- 日志优先级：Error > Warning > 分类异常（Blueprint / Net / Asset）；每条记录 timestamp + category + message，与用户操作时间线对齐；定位到根因后忽略后续派生刷屏。
- 约束：诊断期间不做大重构；先保留失败证据再加缓解；修复与加日志分两步。
- 失败表：不可复现（补前置条件/加检查点）· 日志噪音（收窄分类与时间窗）· 多因并存（拆独立假设，跑低成本判别实验）· 加日志后问题消失（时序敏感，用低开销标记复现）· 只在打包版复现（对比 cook 与编辑器配置、加载顺序）。
- 验证收敛：一条断言能证明就停；失败最多重试一次，第二次仍失败把报错原文交回用户。

## 蓝图

- 链路模式：入口事件 → 守卫 Branch（可选）→ 核心动作节点 → 输出反馈（PrintString / UI）。
- 引脚顺序：先 exec 后 data；动态节点先查引脚再连；每个逻辑块编译一次以隔离故障。
- 键盘/输入：优先专用输入事件操作（复用已存在节点），不要先猜通用节点类；新实现优先 Enhanced Input。
- Enhanced Input 要点：Input Action 与 Mapping Context 都要落位；mapping context 未注册或 action 不匹配 → 事件永不触发。
- 失败表：按键不触发（事件类型 / 重复节点 / 输入焦点）· 编译过但行为错（分支条件与 exec 顺序）· 引脚连不上（变体引脚名不同，先查）· 事件多次触发（重复入口节点 / 重复绑定 → 合并入口 + 状态守卫）。

## UI / UMG / Slate

- 选型：UMG 做常规 HUD/菜单；Slate 做自绘、密集网格、底层输入；hybrid = `UWidget` 宿主内嵌 `TSharedPtr<SWidget>`（`RebuildWidget()` 建实例，蓝图 API 转发数据/配置）。
- 生命周期：construct 里绑定一次，监听器存 handle，destruct 解绑；清 timer/delegate/临时引用；重开不得留重复实例。
- 输入与焦点：显式设 input mode（UIOnly / GameAndUI / GameOnly）；给根控件设焦点；弹窗关闭要**归还焦点**（关闭前存旧焦点 owner）。
- Tooltip/弹窗：锚点 + 固定偏移 + 近边缘翻转 + 最终 clamp 到 viewport；hover 抖动要 debounce，避免闪烁。
- 数据：单一权威来源（subsystem/component/view model）；能事件驱动就别逐帧轮询；显示控件只读，不写游戏状态。
- 失败表：出现但不刷新（绑定时机）· 只刷一次（delegate handle 丢失 / 重建未重绑）· 输入被 UI 吞（input mode 与焦点）· 手柄导航坏（焦点未归还）· tooltip 边缘闪（clamp 与 hover 抖动）· 反复开关内存涨（stale delegate/timer）。

## 世界交互（拾取 / 生成器 / 重叠）

- 五段契约：检测源（overlap / trace / use key）→ 资格与权威（距离、Tag、容量、服务器权威）→ 结算（成功/失败结构化载荷）→ 反馈（VFX/SFX/UI，成功失败都要）→ 生命周期（销毁 / 隐藏禁碰撞 / 冷却 / 重生）。
- 拾取体结构：root + 碰撞球/胶囊 + 视觉组件；碰撞形状与实际可交互距离对齐；通道/配置显式。
- 生成器：候选 ID + 权重 + 数量/半径/最小间距；不重叠采样（被占用或过近就拒绝）；需要复现时显式 seed；重生策略（全清重建 / 增量补齐 / 手动触发）。
- 网络：共享状态的结算在服务器；客户端预测只用于 cosmetic；复制结果状态而不是输入；冷却/计时器归权威端。
- 失败表：overlap 不触发（未开 overlap 事件 / 通道响应）· trace 漏目标（通道配置 + ignore 列表误用）· 多次触发（缺 in-progress 守卫 / 重复绑定）· 客户端成功服务器拒绝（权威校验缺失）· 已消耗还能交互（成功后立刻禁碰撞）· 生成物泄漏（实例记账 + 重置清理）· 密集区掉帧（降频、按输入 gate、休眠关 tick）。

## PCG（程序化生成 / 建筑）

- 六阶段流水线：Input → Filter → Transform → Grammar → Output → Validate；每阶段必须声明：输入数据类型 / 核心节点（≥2）/ 必填参数 / 输出类型 / 调试方法，五缺一即视为设计不完整。
- 参考链（引擎类名，C++/画布对照用）：`UPCGDataFromActorSettings`→`UPCGGetActorPropertySettings`；`UPCGFilterByTagSettings`→`UPCGAttributeFilteringSettings`；`UPCGCopyPointsSettings`→`UPCGApplyScaleToBoundsSettings`；`UPCGSubdivideSplineSettings`→`UPCGSelectGrammarSettings`；`UPCGStaticMeshSpawnerSettings`（默认）/ `UPCGSpawnActorSettings`；`UPCGDebugSettings`。
- 关键属性（进水流）：`LotId`（稳定身份）、`DistrictTag`（风格路由）、`Seed`（确定性）、`FloorCount`（竖向展开）、`GrammarKey`（规则选择）。
- 确定性三条：任何随机分支前先归一化点顺序；所有加权/随机决策绑显式 seed 属性；同一次生成 pass 内 style/grammar key 不变。
- 运行时生成必设：`GenerationTrigger = GenerateAtRuntime`、显式 `GenerationRadii`（别靠默认）、显式 `SchedulingPolicyClass`。
- 调度刷新：5.8 用 execution-source 系列（`RefreshRuntimeGenExecutionSource` / `RefreshAllRuntimeGenExecutionSources` / `DirtyRuntimeGenExecutionSources`），5.6/5.7 用 component 版 `RefreshRuntimeGenComponent`；立即清理 `CleanupLocalComponentsImmediate`；半径缩小或分区归属变化 → 先清理再对受影响范围局部重生成。
- 输出策略：默认静态网格 spawner（ISM/HISM 承担大数据量）；只有需要交互/状态的部分才 spawn actor；输出按层/Tag 分开便于调试与剔除。
- 预算（起步值，不是引擎硬限）：单次更新 PCG 可见耗时 p95 ≤ 4ms；每更新交互 actor ≤ 50；每更新静态网格实例 ≤ 2000；每激活格候选点 ≤ 10000；连续 5 次以上超预算才做优化。
- 优化手法：先减候选点 → 非交互件从 actor 改实例 → 重语法分支降频/拆成延后 pass。
- 失败表：不生成（源/边界/Tag 过滤后候选为空）· 编辑器预览有、运行无（触发方式与半径）· 刷新范围过大（半径与源移动）· 规则收缩后残留（清理路径）· 卡顿（每格点数 / actor 数 / 单次工作量）· 同 seed 结果不一致（上游顺序不稳或随机未绑 seed）· 语法空模块（grammar 串/模块字典/段长约束）· 重叠穿透（前置 clearance/slope 过滤）· navmesh 退化（碰撞与 nav 影响标记，拆开 nav 相关输出）· 改参数不生效（走调度刷新）。
- 验收场景：固定 seed 重放一致 · 源移动一格只重算重叠格 · 半径缩小清理越界结果 · 输出策略从 actor 换实例后 hitch 下降且功能不变 · 换某分区 grammar 只影响该分区。
- 版本差异：5.6 头文件 `Public/PCGSubsystem.h`，5.7/5.8 为 `Public/Subsystems/PCGSubsystem.h`；5.8 新增非破坏式手动编辑与复杂 metadata 属性（当作 5.8 可选能力，不进跨版本图）。

## C++ Gameplay

- 产出契约：`.h` + `.cpp` 成对给；头文件只留必要 include，其余前向声明；UObject 指针用 `TObjectPtr<>`。
- 反射清单：`UCLASS/USTRUCT/UENUM` + `GENERATED_BODY()`；`UPROPERTY` 的 Edit/Blueprint 读写可见性要有理由；动作类 `BlueprintCallable`、只读查询 `BlueprintPure`、`Category` 明确；metadata（ClampMin/AllowPrivateAccess）只在需要时加。
- 复制：`GetLifetimeReplicatedProps` + `DOREPLIFETIME`；客户端可见副作用用 `ReplicatedUsing=OnRep_*`；`Server/Client/NetMulticast` 的可靠性要显式选。
- GameplayTag：显式 tag 名 + 启动校验 + 缺失回退，不写死魔法串。
- 失败表：类编过但蓝图里没有（缺反射说明符/未重生成工程）· 未解析符号与循环 include（重头文件移 .cpp）· 复制不更新（漏注册或 owner 不复制）· `OnRep_*` 不触发（非权威写入或值未变）· RPC 被忽略（调用端权威错）· tag 逻辑静默失效（tag 名/config 错）。

## 存档与同步

- 状态三分类，逐字段标：只持久（SaveGame）· 只复制（运行时同步）· 两者（权威状态 + 存档恢复）。
- schema 契约：显式版本整数；稳定 key（FName/GUID/slot id），不用隐式顺序；旧档迁移路径；恢复顺序 owner → component → 依赖；缺失资产有显式回退；恢复过程尽量幂等。
- 流程：`BuildSaveData()` → 容器（`USaveGame`）→ 落盘（同步或异步，看延迟敏感度）→ 读档校验版本 → 迁移 → `RestoreFromSaveData()`；返回结构化结果（成功/失败原因/版本）。
- 异步回调守卫：状态 token + world 有效性检查，防止与游戏流程切换竞争；部分失败要回滚已创建对象。
- 复制纪律：只复制客户端可见状态；`OnRep_*` 只做客户端重建/刷新，不做权威写；RPC 传意图不传状态；客户端本地存档不能当多人权威输入。
- Iris：5.8 已生产可用，但先探测项目当前复制系统，别假设老项目已迁移。
- 失败表：读档后状态不一致（恢复顺序/缺依赖）· 旧档更新后崩（版本与迁移）· 重连后复制值与存档不一致（先权威恢复再复制）· OnRep 刷屏掉帧（批量/粗粒度复制，细粒度客户端推导）· 客户端越权改动（服务器校验补齐）· 异步回调竞争（token 守卫）· 半途失败留孤儿对象（事务化 + 清理）。

## 性能与打包

- 契约：目标平台 + build config + 测试图；可复现采集场景（相机路径/时长/net mode）；基线与验收阈值；打包配置与依赖扫描范围；go/no-go 结论含明确阻塞项。
- 六步：冻结场景采基线（`stat unit` / `stat gpu`，取中位/分位而非单帧尖峰）→ 热点隔离（必要时加 `TRACE_CPUPROFILER_EVENT_SCOPE` / `DECLARE_CYCLE_STAT`）→ 资产与依赖校验（AssetRegistry：按路径取资产 + `get_dependencies` / `get_referencers`）→ 打包配置校验（BuildConfiguration / MapsToCook / DirectoriesToAlwaysCook / UsePakFile / bUseIoStore / bGenerateChunks）→ 打包试跑（先解第一个阻塞错误，跑完再看级联）→ go/no-go。
- 约束：没有前后测量不下优化结论；报告带单位（ms/fps/内存）与采集时长；区分编辑器开销与打包运行时；发布用显式 cook 列表，不用隐式发现。
- 5.8 注意：Zenserver cooked 输出当迭代存储，不替代 Pak/IoStore 发布校验；Incremental Cooking 属 beta 迭代路径，发布仍要有 clean/full cook；任何打包设置变更后重跑就绪检查。
- 失败表：数据抖动（锁场景 + 预热 + 分位）· 优化不可复现（暴露采集配置，同条件重跑）· 打包错误级联（先解最早阻塞）· 打包缺资产（补 cook 列表 + 修断引用）· 打包比 PIE 慢（对齐 scalability/设备设置）· 内存回归（常驻资产压力与 streaming/cook 策略）。

## 架构与模块边界

- 分层：数据契约（枚举/结构/DataAsset）→ 运行时系统（component/actor/subsystem）→ UI（UMG/Slate 包装）→ 编辑器工具；上层可依赖下层，反向禁止，运行时模块不得引用编辑器模块。
- Build.cs：只有 public 头 include 了的模块才进 `PublicDependencyModuleNames`，实现用的进 Private；重构后删无用依赖（直接降编译与链接开销）。
- 输出物：模块职责与归属表 + 公共 API 最小面 + 风险清单（循环 include、过度暴露反射类型、跨层引用）。
- 需要先停下问用户的场景：改动涉及资产重定向、类改名、包路径迁移，或模块拆分改变了既有蓝图类路径。

## 模块路由（按引擎模块名定位）

- 优先级：精确模块名匹配 > Build.cs 路径段匹配 > 别名/关键词重叠 > 域兜底。
- 置信度：高 = 精确模块命中且映射稳定；中 = 域映射命中且非 General；低 = 兜底到 General + 弱别名证据（低置信只给前 2 候选并要求确认模块名）。
- 映射过期就读不到了：重建走 `ue5-architecture/scripts/generate_module_index_v2.py` 那种「从引擎源码重建索引」的思路，别手工维护大表进上下文。
