# 工作流使用说明

> **开工协议**：开工先在 `RAG/ue-rag`（用该目录 `.venv`）跑 `python agent/resume_context.py`，它打印 [`progress.json`](../RAG/ue-rag/data/harness/progress.json) 的 goal / phase / next_action、[`feature_list.json`](../RAG/ue-rag/data/harness/feature_list.json) 里第一条 `pending`（`passed` 只由裁判或离线检查改写，不认模型宣布）和最新一份交接的 `unmet`，不超过 200 字；进程概览见 [`documents/harness/overview.md`](documents/harness/overview.md)。本文按标题只读需要的那一节。和后文沿革冲突时，以「当前系统」和 `feature_list.json` 为准。

## 当前系统（2026-11-17）

### 一次运行只连一个编辑器

`backends/select.py` 在入口读一次 `UE_EDITOR_BACKEND`，写进该 run 的 `editor_backend` 事件。本进程之后不跟 `.env` 改口。选错入口直接拒绝，原文带当前后端名，不改连另一边。

| 值 | 地址 | 调用 |
| --- | --- | --- |
| `official`（键缺失时的缺省） | `UE_MCP_URL`，缺省 `http://127.0.0.1:8000/mcp` | `scan_mcp_tools.py` → `call_mcp_tool.py` → `core/editor_exec.py` |
| `monolith` | `UE_MONOLITH_URL`，缺省 `http://127.0.0.1:9316/mcp` | `scripts/monolith_backend.py`，或 `backends/monolith.py` 的 discover / describe / exec |

两边同时在听时启动被拒绝：关掉 `.env` 没选的那个插件再重启编辑器。PCG 在 Monolith 上没有命名空间，调用失败，不回退 8000。

裁判种类仍是 `editor_assert`、`blueprint_compile`、`cpp_build`、`automation`。`backend=monolith` 时位置走 `mesh.get_actor_info`，蓝图编译 `blueprint.compile_blueprint`，控件 `ui.compile_widget`，Automation `editor.run_automation_tests`。`backend=monolith` 且动作是 `compile_blueprint` / `bind_blueprint` / `create_widget_blueprint` / `edit_widget_blueprint` 时，`agent/sequence.py` 的 `repair_blueprint` 对同一张蓝图最多修 3 次，第 3 次仍失败就停（「已尝试 3 次，Monolith 不具备该能力」），不改连 8000；对应 `UE_ALLOW_*` 不是 true 时次数是 0，步骤写入 `Plan/`。

只读诊断句（「看看 / 检查 / 核对 / 是否有问题」，且不含「实现 / 新建 / 创建 / 修改 / 摆 / 放置」）由 `agent/diagnose.py` 归到源码里唯一一条 Automation 过滤器，收成已有的 `run_automation_test`；零条或多条都不挑，不扩动作表。

本仓库 `.env.example` 里未注释的项是这台机器正在用的值：`UE_EDITOR_BACKEND=monolith`、模型 `deepseek-flash`、五个图开关、`UE_ALLOW_AI`、`UE_ALLOW_MONOLITH_WRITE`、`UE_DEFAULT_SAVE`、`UE_DEFAULT_PIE` 都是 true。`.env` 不提交。键缺失时的代码缺省仍是 `official`、`deepseek-chat`、图开关 false、save/pie true。

### 做到哪

`feature_list.json` 里仍是 pending 的两条：

- `patrol-presentation`：C++ 状态机（`TickPatrol` / `TickChase` / `TickAttack`）与 `AIAgent.Enemy.Patrol` 自动化已绿（1 passed / 0 failed），不建行为树。2026-10-27 那条「monolith 词表缺覆写节点」的归因**已作废**：真因是 `BlueprintImplementableEvent` 只能由声明它的类自身实现——事件声明在 `UTestEnemyPatrolComponent` 上，`BP_TestEnemy`（父类 `ATestEnemy`）够不到。2026-11-12 已给组件补三个 `BlueprintAssignable` 多播委托（`OnPatrolDyn` / `OnChaseDyn` / `OnMeleeAttackDyn`）并在四处调用点补 `Broadcast`；`BP_TestEnemy` 的 EventGraph 已接 `AddDelegate` + `CustomEvent`，`get_execution_flow` 从 `BeginPlay` 可走到三个绑定节点，PIE 实测日志出现 patrol 表现行 13 次。**剩余：把这条跑出一条带 `evidence_sha1` 的 run 记录**（`bench_record` 只收 `passed=true` 且带印章的条款）；无骨架 Actor 不做动画，受击表现等伤害链路。打开 `UE_ALLOW_AI` 不把这条改成 passed。步骤清单仍留 [`Plan/patrol-presentation.json`](Plan/patrol-presentation.json)，长说明仍是 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`；人签文件不改 `feature_list.json` 里这条的 `passed`。
- `editor-transaction`：没有编辑器自己的 Transaction / Undo。失败时写回的是快照，不是 Undo。

`level-assembly-persist` 已是 done。证据 `data/runs/20261027/20261027-191740.md`：整句在 monolith 下从理解器跑到集成矩阵全过，五条 `editor_assert` 各绑自己的放置扇，关卡经 `list_dirty_packages` + `save_packages` 落盘。金币、平台、门、进度中枢在 `L_NewTest`，HUD 由 C++ `UPlatformHUDWidget` 驱动。已放进关卡的实例要读或改属性时，拆句转入 unresolved，不进执行窗口（见下节 partial）。

### 已经钉住、后文旧句不再作数

下面这些在离线检查里，进了 `.github/workflows/offline.yml`。和后文「仍然没有」里 10 月 27 日之前的句子冲突时，以这里为准。

- **条款绑定**：`eval/check_criteria_binding.py`。20261027-180955 的形状（四扇 `place_batch` 全败）下，内容条款保持 false。`launch_editor` 的 synthetic 判词只翻「证明窗口恰好只有它自己」的条款，翻不动放置条款。翻成 passed 时写下判词 `evidence_sha1`（只含 `ok`、`kind`、`window`、`summary`）；回放时印章对不上，条款回到 false。整句收尾是 `ok` / `partial` / `failed`，写进 run 记录的 `sentence`：有执行窗口失败而条款全过是 `failed`（退出码 1）；有未签 `manual` 是 `partial`（退出码 3）；非 manual 全过、没有失败窗口、没有未签 manual 才是 `ok`（退出码 0）。退出码 2 仍是有活心跳，不是 `partial`。模型文本不改 `passed`。`manual` 只由人写的 `data/attestations/<criterion_id>.json` 翻牌（`criterion_id`、`person`、`signed_at`、`evidence_path`、证据文件字节的 `evidence_sha1` 都要在，哈希不符保持 false）；`editor_assert` 等条款拒绝人签。
- **动作 × 后端**：`eval/check_backend_coverage.py` 的 `COVERAGE`，对照 `core/monolith_bridge.py` 的 `MAPPED` / `UNSUPPORTED`。`place_batch`、`set_scalar_param`、`remove_batch`、`move_actor`、`open_level`、`implement_shooter`、`implement_grenade`、`create_level`、`create_widget_blueprint`、`edit_widget_blueprint` 都是 `both`。`partial` 只剩 `judge_pickup` 和 `set_pickable`：monolith 没有 `get_properties` / `set_properties`，这两行原因文字保持原样，也不编进 `MAPPED`。当前后端是 monolith 时，依赖这两条原语的窗口在拆句末尾丢掉，对应半句留在 `unresolved`，并在当次 `Plan/` 写一行：先清实例再放置，或改走官方后端。官方后端这两条仍可进窗口。`place_batch` 生成时写 props 的路径保留。映射外的原语带原因拒绝，不再是笼统的 `UE_EDITOR_BACKEND=monolith`。
- **写前回滚**：`agent/prop_snapshot.py`，检查 `eval/check_write_rollback.py`。scalar 覆写前记旧值；`place_batch` 新建的 Actor 按名登记，失败时删掉唯一实例；同一实例登记两次只删一次；名字对上不是恰好一个就不删。每条快照带动作、目标 ref、逆操作、幂等键，方式是 `compensating`。同一幂等键只回滚一次。同一 ref 旧值不一致则都不做，写 `manual_intervention`。五类确定故障：写属性中途失败写回旧值；编译失败不消费快照；保存失败把关卡差异留在快照目录；逆操作连接失败写 `manual_intervention` 并不再做下一条；run 被 `sweep_orphan_runs` 改判 orphaned 后，续接先按快照逆序恢复，恢复失败则停。这是补偿，不代替 Undo。`editor-transaction` 仍是 pending。
- **入口占用**：多窗口在打开重定向输入之前把 `UE_RAG_STDIN` 改名为 `.consumed`，改名失败就退出。`agent/run_guard.py` 做同一次占位；30 分钟无心跳的 `running` 改判 `orphaned`，不杀进程。有活心跳时单循环和多窗口退出码 2，打印 `run_id`，不写事件。带 `--force`（两种模式都认）才继续，并在被越过的 run 的事件里写 `run_start.forced_by`（本进程 pid 和该 `run_id`）。原文件不在、只剩 `.consumed`、且没有未过期心跳时，这次调用可以接着用它。单循环不改名。
- **两套写策略**：菜单的 `save` / `pie` 走 `config.run_policy("menu")`，`task_policy` 是它的薄包装，缺省 true，任务写了 `save: false` / `pie: false` 或 `--no-save` / `--no-pie` 才关。DSH 走 `config.run_policy("dsh")`，只读人写的 `data/dsh/session_task.json`，文件没有就拒绝写。两套剖面的 bool 不必相等。图写（蓝图、材质、UMG、动画、Niagara）由 `core/graph_write_policy.py` 一处判定。编辑器写锁在 `core/editor_lock.py`，锁文件仍是 `data/locks/editor.lock`。DSH 和菜单 2/3 的写共用这一把，官方后端和 monolith 互相挡住：并发一个成功一个 busy，失败文本带持有者 pid、backend、`run_id`；900 秒无心跳或伪造 pid 算过期；只读不占锁（`eval/check_dsh_lock.py`）。
- **求解开关**：代码缺省 false（环境变量没写就是关）。本仓库 `.env.example` 写成 true，表示这台机器开着。两句都对，说的不是同一个字段。

### 入口

入口是 `RAG/ue-rag/server/static/entry.html`（由同仓库 `server/entry.py` 在 127.0.0.1:8011 提供；双击 `AIAgentProject\start.bat` 一步起服务并开页）：模式「普通对话」跑 `chat.py`、「单循环」跑 `run_multi.py`、「多窗口」跑 `run_windows.py`，「续接最近」走多窗口的 `--resume latest`。单循环和多窗口共用 `plan_windows`，一张 LangGraph。DSH 仍是外壳，不跑这张图；写策略只读人写的 `data/dsh/session_task.json`。动作表的单一声明源是 `agent/actions.py`（清单不再抄在这里），`integrate_verify` 与 `dispose_residual` 只由代码追加。`apply_spec` 写完源码强制编译（`agent/build_error.py`），错误原文进下一轮，编译轮次 ≤3，执行面每次真编译都落 `data/last_cpp_build.json`。

### 门只认整句（2026-11-11 起）

拆句顺序不变：L0 手写规则 → FPS HUD 规则 → 已学句式 → L1 理解器。变的是门的口径。`rules` / `hud-rules` / `learned` 命中前都要整句盖得住：有半句对不上这条路径声明的动作就返回空，交回后面的模型——不把整句领走后让半句蒸发（纯 FPS HUD 整句仍是 `hud-rules` 五扇链零模型；「加血条子弹，同时加滑铲」这类混句不再被它领走）。闸门落点不同：`rules` 经 `_rules_cover` 过 `_clauses_accounted`，`learned` 自己过一次；`hud-rules` 例外——它的分支直接 `return`，不在 `plan_windows` 里过闸门，hud-rules 靠自身整句匹配：`_clauses_accounted` 在 `agent/fps_hud.py` 的 `hud_windows` 里，认不住整句就返回空、交回 L1/求解。这条直接 `return` 是有意的，不要给 hud-rules 补闸门。纯路径半句（HTML 入口层追加的「。参考资产：/Game/UI/WBP_HUD」「。参考文件：Public/FpsPawn.h」「。参考文档：documents/…」）由 `plan_windows._is_reference_clause` 认成 `constraint` 而不是新需求：只认这三种形状，多带一个字就不算。不加这条的话，拖一个源码附件就会让原本强命中的规则链掉进理解器（实测 `编译工程` 是 `rules`，加一句 `。参考文件：Public/FpsPawn.h` 变成 `planner`），落点也跟着漂——`eval/check_entry_paths.py` 按 5 个样本钉住「带/不带附件 source 与扇数一致」。L1 的 goals 盖不住原句（`unresolved` 非空，或半句没人接）时，不再叫规划者重点一遍动作表，也不拿残缺 goals 直接执行：`UE_SOLVER_ENABLED` 开着就进求解兜底（`plan_windows` 标 `solver-fallback`），关着停在缺口、`unresolved` 进任务书等人表态。人审留一次：求解方案预演（只读）里有写 C++ 或改关卡才出任务书，任务书引用这份方案，不再要求生成器用动作表重写一遍。`run_windows._needs_brief` 只对「窗口已经盖住整句」的确定性来源跳过人审门。

### 求解扇预算

`UE_SOLVER_ENABLED` 缺省 false，人开了才走。预算闸门缺省：编辑器 ≤8 次、元数据 ≤12 次、编译 ≤3 轮、模型 ≤6 次（`UE_SOLVER_*` 可调）；namespace 白名单缺省 `editor,mesh,niagara,blueprint,material`。烧完或闸门拒绝就降级回能力缺口，不假成功。内部守卫（`read_only_plan` / `write_skipped` / 参数形状校准 / 写闸预检）见 `agent/solver.py` 文件头，这里不复述。

保存和 PIE：任务没写时跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`，两边缺省都是 true；`--no-save` / `--no-pie` 才拒绝。要求关卡已存在的动作（`open_level`、`implement_shooter`、`implement_grenade`）认裸名昵称：裸名按尾号数字对现有关卡做唯一匹配（`level3` → `L_NewTest3`，带 `alias_note`），对上多个就暂停问全名；`create_level` 不做这个匹配。技能正文 24 份，在 `.codex/skills`。

### 全域 Spec 内核（2026-11-17 边界升级）

在现有拆句 / 求解之外补了一层「模型产 Spec → 脚本确定性执行 → 只读复核 → 回灌修复 → 沉淀」的通用流水线，覆盖九个域（UMG / 蓝图图 / 动画蓝图 / 材质 / Niagara / 动画&ControlRig / Sequencer / 后处理 / C++）。它不是新的业务图，也不改动作表：

- **共享内核**：`agent/spec_pipeline.py`（断言求值、路径取值、有序 `build_domain`：validate→exec→verify→回灌≤2→终态、沉淀候选、写闸）与 `agent/spec_registry.py`（九域**惰性逐域导入**登记：`describe` / `all_hints` / `recognize` / `build`，单域写坏只让该域不可用、不连坐整表）。
- **域内核**：`agent/{umg,graph,animgraph,material,niagara,animation,sequencer,postprocess,cpp}_spec.py`，统一接口 `KIND / JUDGE_KIND / SPEC_HINT / validate_spec / plan_ops / exec_op / verify_spec / recognize / build`。judge_kind 沿用既有证据种类、不新增：图 / UMG / 动画蓝图→`blueprint_compile`，材质 / Niagara / 动画 / Sequencer / 后处理→`editor_assert`，C++→`cpp_build`。模型只产 Spec；写只经内核 + 写闸 + 只读复核。
- **工具单一事实源**：`agent/spec_toolmap.py`，用 `backends.monolith.discover` 对当前后端做**真机核实**——`data/harness/monolith_tool_inventory.json`（8 个命名空间、729 个动作，material 64 / niagara 129 / animation 201 / level_sequence 8 / blueprint 139 / ui 138 / editor 49 / reflect 1）。`spec_pipeline.default_call` 按命名空间路由：原生命名空间→`backends.monolith.exec`，其余→`exec_tool`（official/bridged）。`lookup(domain, op)` 指到真实 `namespace.action` 才是 verified，指不到的如实登记为缺口。
- **诚实边界（不伪造工具）**：材质图连线、ControlRig rig 图、Niagara 系统模块栈、状态机转换接线走结构化 `unsupported` 暂停；真机核实另发现 `level_sequence` 命名空间**只有 8 个只读动作**，Sequencer 的 `AddBinding / AddTrack / Transform·CameraCut 轨道 / SetPlaybackRange` 无写原语（仅 `animation.create_sequence` + 骨骼轨道关键帧可用）。C++ 只验「编译过 + 符号存在」，逻辑正确性不可只读验。
- **位置（地基，未接线）**：这一层是「兜住能力」的地基，**尚未接进多窗口动作表**——把某域提升成 CATALOG 动作、接 `plan_windows` 路由是后续一步（受证据种类按动作静态映射与写权限限制）。UMG 已用 additive 方式接入 `sequence.repair_blueprint`：`edit_widget_blueprint` 在 fps_hud 不接管、且窗口带 Spec 时走它，否则原路径零变化。
- **守卫**：`eval/check_spec_pipeline.py`、`check_spec_toolmap.py`、`check_spec_boundary.py`、`check_{umg,graph,animgraph,material,niagara,animation,sequencer,postprocess,cpp}_spec.py`，全离线（不连编辑器、不调模型）；每只钉「好 Spec 零错 / 坏 Spec 被判 / kernel 确定性 / adapter 覆盖 / verifier 正反 / route 不误报 / 回灌有上限」。都已进 `.github/workflows/offline.yml`。计划与落地记录见 [`Plan/20261117-1610-UE5全域模型脚本边界总览.md`](Plan/20261117-1610-UE5全域模型脚本边界总览.md) 与 [`Plan/20261117-1630-全域边界升级-落地记录.md`](Plan/20261117-1630-全域边界升级-落地记录.md)。

### 2026-11-17 其余收口（拆句 / 动作表 / 入口）

在 Spec 内核之外，这一轮还把拆句、动作表和入口页的几处缺口收掉。全部有离线守卫、不连编辑器。

- **连接词层**：`agent/connectives.py` + `data/skills/connectives.json`（人写词表）。拆句在标点之外把「然后 / 接着 / 随后 / 最后 / 之后 / 其次」（seq）、「并且 / 同时 / 以及 / 顺便 / 顺带」（parallel）等多字连接词当切点并剥掉；**只做「拆句 + 覆盖」，顺序依赖不自动串**（避免 steps 重排把因果接反）。词表缺失 / 坏 JSON → 退化成只按标点切，与旧行为逐字一致。`plan_windows._clause_rows` 用它切句、`_clause_accounted` 先剥连接词再判残句。`eval/check_connectives.py` 钉住。
- **缺信息先问模型**：`agent/model_fill.py`。拆句 / 执行遇到缺路径 / 蓝图 / 源文件 / 过滤器要暂停前，先用模型补一个形状合法的值（源文件还要求磁盘上真存在）；补得出就当人敲了 `r <值>` 直接往下走，补不出 / 形状不合法 / 没模型 → 原样退回人工暂停，行为与改动前逐字一致。只补「缺信息」，不补「能力缺口」。`run_multi._decide_model_first`、`run_windows._decide_window_model_first` 接它。`eval/check_model_fill.py` 钉住（模型替身）。
- **放灯 `place_light`**：`agent/actions.py` 新增动作（slots `light_type`：点光源 / 聚光灯 / 方向光 / 矩形光，缺省点光源）。按类生成原生 Actor（`/Script/Engine.PointLight` 等，走 monolith `editor.author_map_settings`），放完整 `mesh.get_scene_statistics` 复查该类 +1 再按窗口 save 落盘；不改源码、不放蓝图。整句「打开编辑器，在关卡世界原点添加一个点光源」由 `supervise._plan_rules` 确定性拆成 launch → open → place_light，零模型。配套三处：`launch_only` 不再吞掉放灯半句；`_is_cpp_edit` 不再把「加一个 + 场景名词（灯 / 模型 / 道具…）」当改 C++；路线图依赖里 `place_light` 接最近的 `launch/open/create_level`，`open/create_level` 接最近的 `launch_editor`（冷启动不再同轮派发 open_level 去连 MCP，路线图也不再并排不连）。`eval/check_place_light.py`、`check_cpp_edit_scene.py`、`check_roadmap_deps.py` 钉住。
- **HUD 单读数**：「只加一条血条」这类只命中一类读数的请求，`agent/fps_hud.py` 的 `requested_hud_controls` 只落地它要的那几块（血条 → `txt_Health`，并配图形血条 `HealthBarBG` + `HealthBarFill` 底条 / 填充条 + 数值，不只放一个文本 '100'）；两类以上读数仍走整份七控件合同。支持「删除 / 移除…血条」先删后加或纯删。`_HUD_CLAUSE_RE` 认「上限 / 样式 / 设计 / 仿照 / 常见」等血条收尾半句，单读数请求不再掉进通用受控求解。`eval/check_hud_single_readout.py` 钉住。
- **范围级条款进集成**：求解兜底 / 任务书物化把合同落在范围 `scope["criteria"]`，而 run 级 `state.data["criteria"]` 可能被 stitch 把「没有窗口能证明」的条款摘空。`run_windows._delivery_criteria` / `_all_criteria` 在 run 级、窗口级都空时回落范围级条款，避免「真做成了被记成假失败」；未过的范围条款仍判失败。`eval/check_integrate_scope_criteria.py` 钉住。
- **入口「输入指令卡死」修复（20261116）**：①`btn-send` 的 click 不再裸挂 `send`（MouseEvent 恒为真被当 `resume`，点「发送」实际发的是 `:resume latest`），`send` 开头把 `resume` 收成严格布尔；②任务书人审的「确认」按钮答案是空串，`Channel.ask` 不再把空答案判成 `timeout`（收件箱缺行仍按 `timeout`）；③`_run_query` 在「规划 + 任务书人审」之前先落一条空壳 run（`_RunState` + `_EventBus` + `ui_ask.attach`），规划完成后 `execute_windows(pre_state=...)` 复用同一 `run_id` 填真窗口，不再一次输入落两条 run、页面也不会等 12 秒报「没等到新 run」。`eval/check_entry_send.py` / `check_entry_brief_confirm.py` / `check_entry_run_scaffold.py` 钉住。
- **入口「像卡死」三件反馈（20261117）**：顶栏「用时 / 预估剩余」倒计时（`#g-eta`）、中央后台日志尾巴（`#logpanel` / `#logbody` + `GET /api/uilog/<run_id>`，左对齐轨迹，只读 `data/ui/logs/<ts>.log` 的尾巴、不解析成状态）、规划期右侧「规划中」占位卡（`renderPlanning` / `.plan-card`）；运行状态文案按后端日志推阶段（`derivePhase`）。`eval/check_entry_ui_feedback.py` 钉住（只读静态页）。

### 记分、健康与换机

分数只由 `RAG/ue-rag/eval/check_scorecard.py` 写入 `data/harness/scorecard.json`，终端打一行总分和缺口。没有哈希相符的人签时 A 停在 7。`eval/bench_manifest.json` 里冻结的 10 句不满各 3 轮时 B 停在 16。`skipped` 不进分母，也不算成功。`eval/run_bench.py --live` 在完成记录不足 30 条时不打印百分比，退出码 2。脚本自己存在不加分。对外分数以这次打印的为准。

本机把 `.github/workflows/offline.yml` 里已有的 `python eval/check_*.py` 串起来：`python eval/check_all.py`，摘要在 `data/harness/check_all.json`。CI 仍逐条列出，触发方式不变。

`/api/health` 在 `ok`、`model`、`mcp_url`、`mcp_up` 之外还有 `backend`、`index_present`、`circuit_closed`、`checkpoint_writable`、`disk_free_mb`。后五项各带 `ok` 和 `reason`。不调用模型，不往编辑器写。`mcp_up` 仍是原来的探活。

换机用 `RAG/ue-rag/shortcuts/setup.ps1`：找到 Python 3.12，建 `.venv`，装依赖。`.env` 没有才从 `.env.example` 复制，已有则不动。脚本不自己跑那几条收尾命令，只在末尾打印并列出下一步清单：`python eval/check_all.py`、`shortcuts/ingest.bat`、`python server/entry.py --port 8011`（然后浏览器开 `http://127.0.0.1:8011`），由人按顺序跑。不下载 UE。

成本按 `caller` 分列。没有行的入口不填 0。`caller=dsh` 仍是没有行就省略那一节。`promote_from_route` 的 `source_run` 为空时停在 shadow，不进已提升路由。

## 沿革

给拉取本仓库的人。下面按日期记下已经接上的用法。和「当前系统」冲突时，以那一节为准。2026-10-12 接上三条对话、裁判和检索。2026-10-13 把模型名收进配置，技能卡片补到 19 条，评测表开始合计 token，并放入巡逻敌人的 C++ 半边。同一天补上蓝图只读索引、角色上下文切片、一条写图 hook、失败分类文本表，以及成功后的历史和任务记录。2026-10-14 把「编译工程」「启动编辑器」和「能否拾取」拆成独立步骤：编译工程不要求唯一类，拾取只读裁决。2026-10-15 把「句子 → 动作」从正则升级成四层：L0 快路径（手写规则 + 已学句式）→ L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径、自动插创建前置扇）→ L3 学习回写，见 [`Plan/20261015-1705-自然语言理解前置与归地重构.md`](Plan/20261015-1705-自然语言理解前置与归地重构.md)。2026-10-16 把每次模型调用的模型名、耗时、token、错误和响应里真正有的缓存字段记下来，并用同一三条任务对照了两个模型名。观测页由 `report.py` 从已有 trace 生成，四个 HTTP 路由在 `RAG/ue-rag/server/gateway.py`，不跑第二张图。同一天把 `cpp_build` 收成可取消的一次编译：开始时写下进程、开始时间和 300 秒截止，超时或停止都结束进程并记失败。任务书确认和 `s/c/r` 的选择写入事件 `confirm`。2026-10-17 把 L3 从只记句式扩成技能记忆：`data/skills/learned_skills.json` 是主库，`learned_routes.json` 仍可读；失败模式、澄清问法、蒸馏候选和索引队列都在闸门里面。2026-10-19 在同一张 LangGraph 上补上 Harness 缺的四块：决策字段、扇间接交接文件、开工前默认失败的验收合同、回放通过才允许提升的技能。同一批还把契约里的 `query_actor` 和 `run_automation_test` 登记成只读动作，写属性前留下字段快照。供应商、`base_url`、默认不保存、默认不 PIE 都没有改。计划在 [`Plan/20261019-1130-Harness对照与能力缺口升级.md`](Plan/20261019-1130-Harness对照与能力缺口升级.md)。2026-10-20 按 [`Plan/终极蒸馏方案/升级执行/`](Plan/终极蒸馏方案/升级执行/01-信息层.md) 落了三轮升级：信息层（`documents/harness/` 五份文档、`data/harness/feature_list.json` 与 `progress.json`、`ue-harness-session` 会话规则），约束层（`eval/check_architecture.py` 依赖方向测试、`core/risk.py` 三级风险矩阵、`core/fault.py` 熔断、`eval/check_schema.py` 契约校验），自动化层（`agent/resume_context.py` 开工读档、`harness_role` 角色白名单、`agent/garden.py` 园丁、`data/harness/scoreboard.json` 记分板）。机器状态以 `feature_list.json` 为准。

学习路线、岗位对标和 13 周计划在 [`周计划.md`](周计划.md)。那份文档是目标，不是当前操作手册。项目说明在 [`README.md`](README.md)。10 月 13 日第 8 节之后的缺口在 [`Plan/20261013-1310-第8节之后的缺口.md`](Plan/20261013-1310-第8节之后的缺口.md)。打包和部署只写在 [`Plan/20261013-1545-打包与部署.md`](Plan/20261013-1545-打包与部署.md)，还没执行。

## 现在有三条对话

Cursor 里按 `AGENTS.md` 说话的那条还在。人确认 `.cursor/memory/plans/` 之后，可以用 `run_task.py --query` 把改、编译、测试交给闭环。未确认不调用。这不是菜单 3，菜单 3 也不会变成 Cursor 里每句话的默认动作。

不想开 Cursor 时，起 `python server/entry.py --port 8011`——服务会自己打开浏览器，人只做这一个动作（`--no-open` 关掉自动打开；端口上已有实例时不再抢端口，直接开页面）。三种模式共用同一份检索库和同一个编辑器执行器。`run_agent.py` 还在磁盘上，入口页不再调用它。

| | 普通对话 | 单循环 | 多窗口 |
| --- | --- | --- | --- |
| 入口 | 页面模式「普通对话」，或 `python agent/chat.py` | 页面模式「单循环」，或 `python agent/run_multi.py` | 页面模式「多窗口」，或 `python agent/run_windows.py`。页面起的子进程走 `--plain` 纯文本；「续接最近」直接续接最近一个没跑完的 run，REPL 里 `:resume latest`（或 `:resume <run_id>`）同效 |
| 谁规划 | 检索一次后直接回答 | 菜单 2 与菜单 3 共用 `plan_windows`。口径以「当前系统」的「门只认整句」为准：规则、HUD 链、已学句式先看是否盖住整句，盖不住就交回模型。离线检查仍调用 `plan_sequence()`，那条不叫模型 | 同左。L1 的 goals 盖不住原句时不再叫规划者重写一遍：求解开着进求解扇，关着停在缺口。归地、任务书人审、表外动作只记候选，仍按「当前系统」那一节 |
| 改工程 | 不改 | 不再每步输入 `y`。做不了或失败时暂停，可以补一次路径 | 每扇新的 `thread_id`。消息是这一扇的任务句；有上一扇交接文件时，前面再附一份短 JSON（`unmet`、证据种类、`next`），不附工具正文。写编辑器或 `cpp_build` 的窗口过编辑器闸门串行，只读裁决和只读契约动作并行 |
| 验证 | 不编译 | 和以前的多 Agent 相同。模型不能自己宣布成功 | 集成不叫模型。某一扇失败只重试这一扇。任务书确认后代码追加 `integrate_verify` 跑矩阵验收 |
| 记录 | 不写 trace。人在 `s/c/r` 里选择时，往 `data/events/<同一次 run id>.jsonl` 追加 `confirm` | 每步写在 `data/runs/YYYYMMDD/`（按日期分目录，老 run 原平铺兼容读写）。图内部 trace 仍在 `data/traces/*.jsonl` | 同一目录，按窗口分节，带上依赖和 `{ok, summary}`。另有 `data/runs/YYYYMMDD/<run_id>.json` 机读状态、`data/events/<run_id>.jsonl` 事件流、`data/archive/<run_id>/` 归档 |

目标仍是 [`周计划.md`](周计划.md) 第 2.1 节那句验收：一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人。检索、改 C++、编译、读错误、再改、再测由闭环做。C++ 半边已经是状态机：`UTestEnemyPatrolComponent` 和 `ATestEnemy` 绕出生点巡逻，发现玩家后追击，近距离调用 `OnMeleeAttack`。`AIAgent.Enemy.Patrol` 自动化已通过。行为树不再建。`OnPatrol` / `OnChase` / `OnMeleeAttack` 已从 `BlueprintImplementableEvent` 改成组件上的 `BlueprintAssignable` 多播委托，`BP_TestEnemy` 的 EventGraph 已接 `AddDelegate` + `CustomEvent`；剩余是跑出一条带 `evidence_sha1` 的 run 记录，未签时仍按 `manual` 处理，模型文本改不了 `passed`。菜单开跑时代码可以追加一扇只读 `run_automation_test`，不因此把 `pie` 设成 true。自动条款过了、人工条款还在时，整句记「部分完成」。无骨架不做动画，受击表现等伤害链路。早期读数画面在 `RAG/ue-rag/eval/reports/demo-c-patrol-20261016-124807.mp4`。

`chat.py` 的 `answer`、`nodes.py` 的 `plan_node`、`supervise.py` 的 `_from_model` 和理解器都经 `RAG/ue-rag/core/llm_call.py` 调用模型。模型名每次读 `UE_LLM_MODEL`。键缺失时是 `deepseek-chat`。本仓库 `.env.example` 写的是 `deepseek-flash`。供应商和 `base_url` 没有改。每次调用把模型名、耗时、prompt token、completion token、错误写进 `data/costs.jsonl`。响应里如果有 `prompt_cache_hit_tokens`、`prompt_cache_miss_tokens` 或 `cached_tokens`，一并写下；没有就空着，不估算。trace 里同一次 `plan` 也会带上这些字段。

## 一句多步

菜单 2 是单循环，菜单 3 才是多窗口。两边都还是原来那张 LangGraph，没有第二张业务图。图的边没有加。模型不能自己宣布成功。路径对不上仍然不猜。

菜单 2 现在和菜单 3 共用 `plan_windows`、动作表和编辑器执行器。手写规则、HUD 链、已学句式只认盖得住的整句；盖不住的半句交回模型。理解器的 goals 盖不住原句时，求解开着进求解扇，关着停在缺口，不把残句交给老规划者执行。`plan_sequence()` 本身仍只走手写规则和已学句式，离线检查用它，不叫模型。每一步还是同一张图。

菜单 3 的拆句分四层（2026-10-15 起）：模型在前只做理解和选动作，代码在中做归地和裁判，跑赢后沉淀成快路径。手写规则不删，退役成种子和 L1 失联时的兜底。

```text
用户一句话
  │
  ▼
L0 快路径（离线零模型）
  手写规则强命中 / learned_skills.json 已提升技能 / learned_routes.json 同构句式 → 直接 L2 归地
  弱命中（拆出暂停扇）或未命中 ↓
  │
  ▼
L1 理解器（agent/understand.py，一次模型调用）
  输入 = 原句 + 动作卡片 + 3 条检索标题（1KB 上下）
  输出 = 意图 JSON：goals（动作 + 槽位原始 token + 参数）、constraints、unresolved
  只选表内动作，不编路径；说不出合法意图（重试上限 1 次）→ 老规划者（窗口清单 JSON）
  │
  ▼
L2 归地（agent/intent.py 验收 + agent/grounding.py 解析，纯代码，不猜）
  action 必须在动作表里，槽位必须已声明，表外拒收
  asset 槽位：索引唯一命中 → 直连；miss 且父类可推（或给了 /Game 路径）→ 自动插「创建蓝图」前置扇
  推不出 → 暂停问一次 r
  │
  ▼
按 depends_on 跑
  每扇 = 现有 run_task，新的 thread_id
  消息 = 这一扇的句子；有交接文件时前面再附 unmet 和证据种类
  写编辑器或 cpp_build 的窗口共用一把锁
  │
  ▼
L3 学习回写 + 集成（都不叫模型）
  整句集成成功且 unresolved 为空 → 先离线回放
  回放的动作序列不一致，或 unresolved 变多 → 停在 shadow，不进 L0
  回放一致（或没有可对的源 run）才写 learned_skills.json，并继续兼容写 learned_routes.json
  规划者刚拆开、还没跑赢的句式只进影子技能，不参与 L0；没有 run id 的提升也停在 shadow（`source_run` 为空不写 `learned_skills.json`）
  下次同构句子在 L0 命中，零模型调用；技能 LRU 上限 200，旧路由仍是 50
  全部依赖满足且裁判 ok，整句才 ok；某一扇失败只重试这一扇
  自动条款都通过、仍有 manual → 整句记「部分完成」
```

动作表的单一声明源是 `agent/actions.py`（清单不再抄进「当前系统」）。schema 用 `contract/tools.json` 里已有的那一份，不读 `ue_mcp_index.json`。`query_actor` 和 `run_automation_test` 不在 `allow_writes` 里也能读，写前缀规则没有放宽。其余四个契约工具（`query_blueprint`、`find_asset`、`build_project`、`get_build_error`）不进动作表；`get_build_error` 读的 `data/last_cpp_build.json` 现在由执行面的每次真编译共同落盘（2026-10-28 起）。官方 830 个工具只在 `backend=official` 的扫描路径；`backend=monolith` 走 discover / describe / exec。表外动作暂停，记成候选，不执行，也不写进快路径。诊断句归地失败也不为此加行。人审并补上验证之后才算入表。可实现指槽位已经唯一，不是指场景里一定有实例，也不是指编译已经通过。场景里没有实例、编译失败、裁决是「不能」，都不删已经记下的句式。人要作废就删 `learned_skills.json` 里的那一条；旧的 `learned_routes.json` 仍可单独删。

拆句结果上的 `criteria[]` 由代码写，不由模型写。每条有 `id`、`text`、`evidence_kind`、`passed`。`evidence_kind` 只允许 `cpp_build`、`blueprint_compile`、`editor_assert`、`automation`、`manual`，开始时 `passed` 全是 false。`observe` 只在 `judge.ok` 且 `judge.kind` 等于 `evidence_kind` 时把对应条款改成 true。`manual` 保持 false。模型回复里的「已完成」不进这张表。「编译工程」只让 `cpp_build` 那条有机会通过。句子里同时有「巡逻」和敌人（`敌人` / `Enemy` / `TestEnemy`）时，代码追加 `automation` 条款，过滤器 `AIAgent.Enemy.Patrol`，并追加一条留给 `Plan/` 的 `manual` 条款。已有离线拆句期望看的是动作序列，所以这一扇只在菜单执行时追加，不改 `plan_windows` 交给语料检查的那张表。句里写了测试时，原来的 PIE 扇仍按原规则出现；巡逻 Automation 自己的 `pie` 保持 false。

`set_pickable` 要达成 `SetPickable(true)`：实例 `bPickable` 为真，且带 `InteractableActor`。当前工具目录里没有调用 `BlueprintCallable` 的工具，所以对实例写这两个字段，写完再读。不把「写过了」当成成功。写对了就按任务策略保存当前关卡（缺省保存）。场景里没有这个实例则这一扇失败，依赖它的裁决不跑。裁决说「不能」是结果，整句仍然算跑完。

下次同一句式，`plan_sequence` 在开口叫规划者之前就拆开。例如先学会「给场景里的蓝图打上可交互标签，再让另一个蓝图与它交互」，下一句换成别的 `BP_` 名字，只要动作词还在、蓝图或类的个数相同，就直接得到 `set_pickable` 和 `judge_pickup`。

菜单 2 的手写规则仍是下面这张：

```text
一句话
  │
  ▼
plan_sequence 手写规则
  ├─ 编译工程          cpp_build。不改源码。失败留下原文，不进 fix
  ├─ 启动编辑器        句里写了才启动。等到 MCP，最多 120 秒
  ├─ 改唯一 C++ 类     加函数 / 修编译错误。仍可进 fix
  ├─ 编译已点名的蓝图
  ├─ 移动已点名的 Actor
  ├─ 绑定拾取者 / 物体  各自唯一。蓝图：句中路径 → 索引唯一命中 → 否则问一次 r
  ├─ 放进当前关卡      两个槽都绑上才放。缺的才放。放完按任务策略保存关卡
  └─ 只读裁决          bAllowPickup、bCanBePicked、InteractableActor、距离 ≤ 100 uu
        │
        ▼
   能落地的步骤 → 现有图
        retrieve → plan → execute → build → test → observe
                                              └ 失败且未冻结源码 → fix → plan
```

「编译工程后启动 UE 编辑器，然后测试 ATestActor 能否被 ATestPawn 拾取」现在走成：

| 步 | 结果 |
| --- | --- |
| 编译工程 | `cpp_build` 退出码 0 才继续。不调用改源码的工具 |
| 启动编辑器 | 编译通过后启动。记录里留下启动说明 |
| 绑定拾取者 | 索引里的 `/Game/BP_TestPawn.BP_TestPawn` |
| 绑定物体 | `ATestActor` 的蓝图不在索引里。暂停，只问一次 `r /Game/...`。补上的路径若还没有资产，按该类新建再放入，父类去掉 `A`/`U`/`F` 前缀。不改图 |
| 放置、裁决 | 物体没绑上就不跑。跳过这一格也不把「没测」写成「能」或「不能」 |

裁决四条都满足才是「能」：`bAllowPickup`、`bCanBePicked`、带 `InteractableActor`、与拾取者距离不超过 100 uu。缺读数、场景里没有实例、MCP 连不上，步骤失败，不写成「不能」。不写属性，不补标签。

2026-10-14 跑过这句，记录在 `RAG/ue-rag/data/runs/20261014/20261014-121440.md`：编译成功，编辑器说明是「已启动 UnrealEditor」，拾取者绑到 `BP_TestPawn`，物体槽暂停。`Source/AIAgentProject` 的 `.h` / `.cpp` 哈希与跑之前相同。`python eval/check_routes.py` 打印 `routes ok`。当前关卡是 `L_NewTest`。关卡里已有的 `BP_TestActor` 距离 500 uu 时裁决为「不能」，原因是距离，`bPickable` 和标签与裁决前相同；拿掉标签、距离 40 uu 时裁决为「不能」，原因是没有 `InteractableActor`，`bPickable` 仍为真。验证时放进关卡的 `BP_TestPawn` 没有保存，重载 `L_NewTest` 会去掉它。标签已写回 `InteractableActor`。

## 多窗口的升级：面板、任务书、续接、矩阵（2026-10-14 下午）

按 [`Plan/20261014-1633-工作流差距与升级计划.md`](Plan/20261014-1633-工作流差距与升级计划.md) 的六段做完，全部在现有进程模型里，不动图的边，不引入新依赖（面板用已装的 `rich==15.0.0`），不给任务书生成器和规划者看工具 schema。

| 层 | 现在怎么做 | 数据落在哪 |
| --- | --- | --- |
| 预处理（任务书） | 拆出超过 3 扇，或句子里说了「新功能/重构」，或理解器把半句扔进了 `unresolved`（2026-10-15：没着落的半句正是「尚未完成」清单该记的），先出任务书：任务范围、「尚未完成的全部主要内容」清单、估时区间、依赖、外部依赖，自动带总工作量和三行免责。人审门：确认 / `e` 命令就地编辑（改完立即写回同一对 .md+.json，`e help` 看写法，手改计进 `manual_edits`，确认时记「通过，附修改」）/ 写修改意见（按意见重出下一版，记审阅轮次）/ 放弃。审阅界面是 rich 表格（范围、交付、现状、依赖、窗口、估时），不是裸 markdown。窄任务走原快路径，不生成任务书。确认后的范围窗口按 id（或唯一动作）**原样继承**快路径候选，已解析的路径、父类、依赖一并保留，生成器不再重编动作/槽位；没归类的候选窗口照跑不丢（2026-10-15） | `data/briefs/<时间戳>-<slug>.md` + 同名 `.json` |
| 观测（面板与事件流） | 菜单 3 默认 `--board`：顶上主代理区（指令、阶段、审阅轮次、「● 修改了文件」绿点；有蒸馏候选时只显示个数，默认不展开），左下运行窗口区（`子代理 · 可续接`、旋转符 + 当前图节点、最近输出；交接的 `unmet` 写在这扇的备注里），右下完成区（`✓ N 已完成`，按 `E` 展开明细）。`--plain` 保留纯文本；面板不吞 stdout，echo 走订阅者，run 记录照常落盘 | `data/events/<run_id>.jsonl`，一行一事件：`run_start / intent / confirm / window_state / tool_call / judge / context_slice / handoff / files_modified / window_result / integration / learned_route / skill_promoted / index_stale / index_auto_refresh / action_candidate / distill_candidate / run_end`。`intent`、`learned_route`、`skill_promoted` 带 `skill_id`、`version`、`status`。手写规则命中时 `skill_id` 为空，`source` 是 `rules`。`window_result` 和 `run_end` 带 `stop_reason`：`ok`、`paused`、`retry_exceeded`、`read_stall`、`judge_failed`、`unresolved` 之一。`context_slice` 只记计数：`user`、`judge_fail`、`tool_kept`、`tool_dropped`，不记工具正文。拒绝时的 `tool_call` 带 `policy`：当前的 `save`、`pie`、`allow_writes` 和五个 `UE_ALLOW_*`。`confirm` 在任务书确认或 `s/c/r` 选择时写。`index_stale` 在本 run 真新建了蓝图资产时写。编辑器关着就把 `shortcuts\asset.bat` 写入 `data/index_queue.jsonl`，`UE_INDEX_AUTO_RUN=1` 才真正拉起；编辑器开着只提示人执行，不关编辑器。续接不重复提示 |
| 持久（可续接） | 每扇状态写穿机读 run 记录（窗口定义、thread_id、已发布工件、审阅轮次都在里面）。跑到一半 Ctrl+C 或断电，`python agent/run_windows.py --resume latest`（或具体 run id）：`done` 扇直接复用结果；中断扇用原 `thread_id` 从 checkpoint 续上下文；`paused` 且有动作的扇进人工裁决；`paused` 但没绑上动作的旧死桩扇直接跳过并写明——裁决不问它，`r` 也填不活，重拆那句话才会走新路由。「最近一次」不靠 mtime，是把平铺老文件和日期子目录里的 run 按 run_id 排出来取最新，菜单 4 和 `:resume latest` 都走这条（2026-10-15 目录改日期分层后补的）。启动时面板会提示可续接的 run | `data/runs/YYYYMMDD/<run_id>.json` |
| 交接 | 每扇结束用这一扇的窗口结果和两份裁判拼 `did`、`evidence`（`kind`、`ok`、截断后的 `summary`）、`unmet`、`next`、`artifacts`。`unmet` 最多 8 条，同时写 `unmet_total` 和 `unmet_omitted`（为 0 也写 0）。下一扇的短 JSON 带这两个整数，不带第 9 条以后的正文。磁盘上的交接另存全量 `unmet_ids`，只有条款 id。不调用模型，不写工具正文。当天 `.cursor/memory/任务记录/YYYY-MM-DD.md` 已经存在时，才把里面的未完成标题读进 `next`；没有文件就不编。删掉交接目录后，续接仍靠原来的 checkpoint，只是消息里少这一段 | `data/handoffs/<run_id>/<window_id>.json` |
| 容错（快照与裁决） | 会写源码的窗口在编辑器闸门内先拍 `Source/AIAgentProject` 的字节快照，失败且任务无 `keep_source` 时还原；续接前先还原旧快照，半截改动不当前提。`set_pickable` 和 `move_actor` 写之前把将要改的字段读进 `props.json`，这一扇失败且没有 `keep_source` 时写回这些字段，写回后按任务策略保存关卡（缺省保存）。同一引用出现多次就不写回。`paused` 窗口对齐单循环的 s/c/r，只问一次：`s 停止 / c 跳过 / r /Game/目录 或 r Public/类名.h`，裁决问题和输入提示同步进面板「最近输出」。环依赖开跑前报「环：W1→W2→W1」；悬空依赖报「未知窗口 id」，不再拖到集成段 | `data/snapshots/<run_id>/`，属性在同目录的 `props.json` |
| 调度（只读并行） | 依赖全 `done` 的窗口进就绪集；只读动作（`judge_pickup` / `check_pickup` / `query_actor` / `run_automation_test`）共享读闸并行，写场景/编译/PIE 独占写闸，读写互斥、写写互斥。规则窗口不再假链：按名字数据流连依赖（BP_TestActor 与 ATestActor 算同一目标），独立目标不虚连；规则拆出的测试窗口接在最近的编译/改码扇后面，上游没绑上动作、解析失败暂停时同样接上。后来补出来的 PIE 窗接在最后一扇后面，上游暂停时也不抢跑。成功扇按名字发布工件 `{ref, in_level}`，下游按槽位名订阅，放置→改名→裁决这类多物任务传得了 | 事件流里的起止时间戳可证重叠 |
| 验收（合同、矩阵与台账） | 拆句时代码写下 `criteria[]`，任务书确认后每个范围也带一份，`passed` 从 false 起。对得上的裁判才能改 true。任务书确认后另由代码追加一扇 `integrate_verify`（规划者写不出它）：重复请求幂等（重跑一次最近成功的只读裁决，结论不变才过）、拒答路径、延迟窗口、断线重连、未知态恢复，一项一个子判定；产出触及文件哈希、窗口 elapsed/retry/token 汇总和归档。失败扇无下游依赖时整句记「部分完成」。自动条款都通过、仍有 `manual` 时也记「部分完成」。报告按范围列「A 完成；B 失败」。任务书另有一节尚未完成清单，来自台账、未提升技能和 unresolved | `data/archive/<run_id>/`；范围级台账 `data/ledger.json` |

台账 `data/ledger.json` 只给机器读：范围级 `done / partial / pending`，由集成结果回写，任务书生成器的「尚未完成的全部主要内容」从这里取。给人的记录仍在 `.cursor/memory/任务记录/`，两边不互相替代。估算只是记录字段，代码自动带免责三行，执行层不读、不作为预算、超时不杀窗口。

REPL 里 `:quit` 退出，`E` 展开上一个 run 的每扇明细（状态、ok、elapsed、retries），`:resume latest`（或 `:resume <run_id>`）续接——误敲成 `--resume ...` 或裸写 `resume latest` 也按续接处理，别的 `--xxx` 只提示不执行，不再当任务句进规划者（2026-10-14 185550、190429 两次白烧规划者调用后补的）。入口页「续接最近」直接续接最近的 run。续接时面板读得到上一扇的 `unmet`；交接文件被删掉也不影响 checkpoint。`eval/check_routes.py` 现在同时跑 `eval/check_upgrade.py`：离线覆盖六段（事件流与机读状态、面板渲染、任务书与人审门、续接与快照还原、环/悬空诊断、只读并行、工件、矩阵台账归档）加 build 前打转熔断与 REPL 指令，通过时打印 `upgrade ok` 和 `routes ok`，不叫真实模型、不写场景。蒸馏契约在 `eval/check_distill.py`，通过时打印 `distill ok`。Harness 字段、合同、交接、上下文、回放、只读动作和属性快照在 `eval/check_harness.py`，通过时打印 `harness ok`。`python agent/distill.py --dry-run` 只统计候选，不叫模型。`UE_DISTILL_TEACHER=1` 且不是 dry-run 时，才用现有 `UE_LLM_MODEL` 把候选收成技能 JSON，追加进 `data/distill/candidates.jsonl`，不写 `learned_skills.json`。巡逻敌人的蓝图步骤写在 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`，不自动绑图。

## 分工

核心玩法走 C++，进多 Agent 闭环：`read_source`、`apply_unified_diff`、`cpp_build`。加函数或修编译错误时失败进 `fix`，并且要求唯一的 C++ 类。句里给了唯一的 `.h` / `.cpp` 文件名时（可以带 `Source\...\Public\` 这种整段路径），目标类由文件钉死，句中提到的其余类名（如括号里的参数类型 `ATestPawn`）只当引用参数，不算第二个候选；带分隔符的整段路径不参与抽类名，路径里的目录名（如 `AIAgentProject`）不当类名。没给文件名时仍要求整句只有唯一类。「编译工程」「编译项目」「编译一下工程」并且句里没有改函数时，步骤是整份工程的 `cpp_build`，不要求唯一类，不改源码；失败留下错误原文，不进 `fix`。句里写了「启动编辑器」「打开编辑器」或「启动 UE」时，编译通过后启动编辑器并等到 MCP，最多 120 秒，不要求这次写过源码。连接超时不算已连通。句里同时在改函数时，仍走改源码那一步，不被「编译工程」这条例外吃掉。PIE 里要找的对象只认 C++ 索引里真有的类，父类是 Pawn/Character 的优先；没有 Pawn 类时句中唯一的 `BP_` 名（会被放进场景的那个）优先，一个都解析不到才退 `TestPawn`，不再拿句首第一个像类名的词去场景里找。

同一句里如果还有「创建蓝图」，但没有 `BP_` 或 `/Game/` 路径，这一步会暂停。创建路由由「新增/创建/新建动词后面直接跟 `BP_` 名、`/Game/` 路径或『蓝图』」触发：句里给了完整 `/Game/` 路径就直接绑好「建蓝图」（父类用句中「继承自」的那个类），句里说了放到场景就再绑一扇「放进场景」，都不再问 `r`；没给路径才照旧暂停等一次 `r`。「给某类新增一个属性」这类动词后面跟的不是蓝图、蓝图只是顺带要编译的，不触发创建路由，仍走改 C++ 加编译那张蓝图。「编译代码」和「编译工程」一样算整份工程的 `cpp_build`，不改源码。凡是因为不知道目录或源文件而停住的步骤，都只给一次 `r` 补路径：蓝图用 `r /Game/某目录` 或 `r /Game/某目录/BP_名字`，C++ 用 `r Public/类名.h` 或 `r Private/类名.cpp`。只写蓝图目录时，资产名按类名，例如 `ATestActor` 得到 `BP_TestActor`，父类是 `/Script/AIAgentProject.TestActor`。创建蓝图之后的「放进场景」沿用这次填的路径，位置 `(0, 0, 100)` 并贴地，不保存关卡。

没有创建动词的「把已点名的蓝图放进场景」也接上了（2026-10-15）：句里写明放到/放进场景（含「放一个到场景中」「再放一条进关卡」这类带量词和「再」的措辞）、点的是 `BP_` 名或 `/Game/` 路径就触发。索引里唯一命中的直接放；索引里没有的，句中唯一的 `BP_` 名加最多一条 `/Game` 路径，父类按 `BP_` 名从 C++ 索引推（`BP_TestEnemy` 对应 `ATestEnemy`），推不出唯一候选就暂停问一次 `r`，不猜。路径上还没有资产时按该类新建再放，不改图。放置位置：场景里已有同名实例时，读它位置后错开 50 uu 放、不贴地（不叠一起）；没有才放 `(0, 0, 100)` 贴地。这一句子里若同时有「编译代码」，编译只算整份工程那一格，不再误拆出一扇必停的「编译蓝图」。

拾取按槽位，不进 PIE，也不走 Automation。`A 能否被 B 拾取`、`B 拾取 A`、`被 B 拾取` 里，B 是拾取者，A 是物体。对不上就暂停，不猜角色。每个槽在 C++ 索引里要唯一。蓝图先用句中路径，否则用索引里父类和资产名都对得上的唯一蓝图，再没有才问一次 `r`。人给出的路径上如果还没有资产，放置前按这个类新建蓝图并编译，父类是 `/Script/AIAgentProject.TestActor` 这种去掉前缀的反射名，不改图。不按 C++ 类直接生成实例。两个槽都绑上之后，缺的实例才放进当前关卡：都缺时分别放在 `(0,0,100)` 和 `(50,0,100)` 并贴地；只缺一个时放在已有实例旁边 50 uu，不贴地，避免被地面拉开。不保存。只读裁决见上一节。已经点名的蓝图（例如 `BP_Gun`）和 C++ 类写在一句里时，先改 C++ 再编译那张蓝图。标签字符串即使以 Actor 结尾，对不上索引里的类就不算类名。加函数时允许改这个类的头文件和 cpp，先改再编译，不先把「当前已经能编过」当成做完。修编译错误仍是先看到失败再改。新逻辑拆成 Component、Subsystem 或 Controller，不堆进一个 Actor。表现钩子用 `BlueprintImplementableEvent`。

带 `cpp_build` 的任务开始前，如果 `UnrealEditor.exe` 已在运行，会先关掉它再改源码、再编译。未保存的编辑器改动会丢失。编译前若编辑器又被拉起，会再关一次，避免游戏模块 DLL 被占用。闭环再打开编辑器时带 `-AutoDeclinePackageRecovery`：强制关掉不是正常退出，否则会弹出 Restore Packages，人没点之前 MCP 起不来。人手双击工程不受这个参数影响。

源码改动留下且编译通过后，闭环调用 `shortcuts\cpp.bat nopause`：`export_cpp_metadata.py` 再 `ingest_assets.py`。手工要刷索引时双击 `shortcuts\cpp.bat`，它会暂停等你看结果。然后启动 `UnrealEditor.exe`。这次如果是我们把编辑器关掉的，任务结束也会再打开，避免编辑器一直关着。菜单 1 不关编辑器，也不启动编辑器。菜单 2 和菜单 3 在要编译 C++ 时会先关编辑器；句里写了启动编辑器、并且这一步通过后，才会再打开。

蓝图改图看 `UE_ALLOW_BLUEPRINT`（`RAG/ue-rag/.env`，缺省 `false`）。`false`：不改蓝图，把操作步骤写成 `Plan/YYYYMMDD-HHMMSS-蓝图名.md`（打开哪张图、从哪个事件的哪根引脚连到哪个节点），然后停。`true`：可以改蓝图 Graph，用 `blueprint_compile` 当裁判，不保证一次做对；失败进 `fix`，达到 `max_retries`（缺省 3）后停止。材质图、UMG、动画蓝图、Niagara 另有四个开关，缺省 `false`：`UE_ALLOW_MATERIAL`、`UE_ALLOW_UMG`、`UE_ALLOW_ANIM`、`UE_ALLOW_NIAGARA`。不是 `true` 时不改这些图。代码缺省仍是 false。本仓库 `.env.example` 正在用的值把五个图开关、`UE_ALLOW_AI`、`UE_ALLOW_MONOLITH_WRITE` 都写成 true，后端是 `monolith`。`eval/check_risk.py` 只读比较开关和 `data/project_context.json`，漂移时提示重跑 `pipelines/export_project_context.py`，不改开关。`backend=monolith` 下蓝图类动作的 3 次止损见「当前系统」。

Cursor 外壳另有一条 `beforeShellExecution` hook：`.cursor/hooks.json` 调用 `.cursor/hooks/refuse_graph_writes.py`。Codex 用 `.codex/hooks.json` 的 `PreToolUse`（`Bash` 或 `shell`）调用 `.codex/hooks/refuse_graph_writes.py`，判定仍是同一份 `decide`。命令行里出现 `call_mcp_tool` 时，`write_graph_dsl`、`create_node`、`connect_pins`、`add_event`、`add_variable`、`set_pin_value` 在开关不是 `true` 时拒绝；读不到 `.env` 也拒绝。材质、UMG、动画（含 Control Rig）、Niagara 的写工具看上面四个变量，缺省拒绝；`get` / `list` / `find` / `read` / `search` / `discover` 开头的读工具不看这四个开关。闭环里的拒绝读同一组 `UE_ALLOW_*`：`official` 在 `editor_exec.py`，`monolith` 在 `backends/policy.py`。这不把 `--query` 接进每句话。Codex 钩子要先在会话里 `/hooks` 信任。

仍自动做的编辑器事：移动 Actor、读属性、编译一张已经存在的蓝图。编译不改图。

蓝图还在引擎里。留给人的是连线和资产引用。

## 框架

```text
server/static/entry.html ─┬─ 普通对话 chat.py ─ retrieve ─ 回答
                            ├─ 单循环 run_multi.py ─ plan_windows（与多窗口同一条）─ 每步同一张图
                            └─ 多窗口 run_windows.py ─ 规划者/任务书/快路径 ─ 每扇 run_task（只读并行过编辑器闸门）
                                          board.py 面板 · brief.py 任务书+人审门 · ledger.py 台账
                                          harness.py 合同/交接/停因 · prop_snapshot.py 属性快照
                                                            │
                    ┌─ Cursor ─ official: scan / call_mcp_tool ─┐
                    │         monolith: monolith_backend.py           │
                    │         任务记录里已有的未完成标题，菜单开跑时才读 │
任务 JSON 或一扇 ───┤                                                 ├─ backends/select.py 锁一次
                    │                                                 │    official → editor_exec.py → :8000
                    └─ run_task.py ─ LangGraph ───────────────────────┘    monolith → backends/monolith.py → :9316
                              │
                              ├─ llm_call.py    每次模型调用的用量。gateway 也走这里
                              ├─ plan_sequence  手写规则，然后 learned_skills.json / learned_routes.json
                              ├─ plan_windows   L0 → L1 理解器 → L2 归地 → 老规划者兜底
                              │                 诊断句在归地前收成 run_automation_test
                              ├─ distill        只读 run/event/trace/archive。回放不一致不提升
                              ├─ intent/understand/grounding/diagnose  意图验收、一次模型理解、asset 槽位、诊断过滤器
                              ├─ retrieve  路由，未命中再 BM25 + bge。不进规划者
                              ├─ plan      按 verify 选 C++ 或编辑器角色。发出 context_slice 计数
                              ├─ execute   本地工具，或当前后端。拒绝带 policy。monolith 蓝图最多 3 次
                              ├─ build     blueprint_compile / cpp_build（裁判，无 LLM）
                              ├─ test      editor_assert / automation，没有则 skip
                              ├─ observe   只看裁判 ok，并按 kind 改合同。freeze_source 时失败即结束
                              └─ fix ─→ plan     只编译工程的步骤不进这里
```

分层：

| 层 | 模块 | 职责 |
| --- | --- | --- |
| 配置 | `config.py`、`.env` | 引擎、工程、源码根、后端、MCP 地址、执行模式、超时、模型名 `UE_LLM_MODEL`。路径缺省仍是本机。模型名缺省 `deepseek-chat`，`.env.example` 写 `deepseek-flash` |
| 语料 | `pipelines/export_*.py`、`ingest_assets.py` | JSONL → Chroma `ue_assets`。本次出现过的 `source_type` 会删掉不在本次 id 里的旧条目 |
| 工具目录 | `scan_mcp_tools.py`、`ue_tools`、`monolith_backend.py` | 官方卡片无 schema。`--schema` 才读索引里的一个工具。monolith 用 discover / describe，不传 `detail=true` |
| 执行 | `backends/select.py`、`core/editor_exec.py`、`backends/monolith.py`、`core/monolith_bridge.py` | 一次运行锁一个后端。`official` 走 `editor_exec.py` 到 8000。`monolith` 走 discover / describe / exec 到 9316；场景/资产/材质原语经 `core/monolith_bridge.py` 翻译成对应 monolith action（写策略按 official 原语名检查不变），桥没映射的原语带原因拒绝。选错入口拒绝 |
| 本地补丁 | `core/source_edit.py` | `read_source`、`apply_unified_diff`。路径锁在 `Source/AIAgentProject` |
| 裁判 | `core/judge.py` | `{ok, kind, summary, errors:[{file, line, message}]}` |
| 图 | `agent/graph.py`、`agent/nodes.py`、`agent/roles.py` | 节点和边不变。`plan` 按任务种类换提示和工具。trace 记 `role` |
| 监督者 | `agent/supervise.py`、`agent/plan_windows.py`、`agent/catalog.py`、`agent/intent.py`、`agent/understand.py`、`agent/grounding.py`、`agent/diagnose.py`、`agent/sequence.py`、`agent/skill_memory.py`、`agent/failure_memory.py`、`agent/clarify_policy.py`、`agent/distill.py`、`agent/harness.py`、`agent/roles.py`、`agent/garden.py`、`agent/resume_context.py` | L0 手写规则（每步都落地才算强命中）先拆，然后已提升技能，再然后 learned_routes。弱命中/未命中过 L1 理解器（动作卡片 + 原句 + 3 条检索标题，1KB 上下），意图经 `validate_intent` 验收后 L2 归地：asset 槽位 `link_asset` 索引命中直连并看索引是否过期、类可推自动插创建前置扇、推不出问一次 `r`。只读诊断句在补窗前由 `diagnose.py` 收成唯一过滤器的 `run_automation_test`。L1 失败才叫老规划者（看不到工具原文），重试上限 1 次。整句集成成功、unresolved 为空、并且回放没有变差，才提升技能（提升必须带 `source_run`，空值停在 shadow；提升写 `source_runs` 数组和 `content_sha1`）。`fix` 先查 `failure_patterns.json`，症状对不上再用 `ue_error`。蒸馏读 run、event、trace 和 `data/archive` 的哈希与矩阵结论，失败样本保留 `error Cxxxx`。默认不叫教师模型。任务可带 `harness_role`：`research` / `review` 只读，写工具一律拒（`allow_writes` 里写了也拒）；`plan` 只返回窗口清单；没写时行为与原来完全一致，仍是同一张图。`garden.py` 只扫文档抬头、记 stale，`resume_context.py` 开工读档。`sequence.repair_blueprint` 在 monolith 下对蓝图类动作最多 3 次 |
| 网关 | `server/gateway.py` | 127.0.0.1:8010 上四个路由：`/api/health` 返回模型名和 MCP 是否可连；`/api/chat` 一次补全；`/api/structured` 包理解器；`/api/tool-call` 只转发契约里的六个工具。不拆窗口，不跑 LangGraph |
| 入口层 | `server/entry.py`、`server/prompt_builder.py`、`server/static/entry.html`、`agent/ui_ask.py` | 127.0.0.1:8011 的 HTML 入口（`python server/entry.py --port 8011`；`AIAgentProject\start.bat` 双击一步起）。**读**：`/api/runs`、`/api/run/<id>`、`/api/stream/<id>`（tail `data/events/*.jsonl`，`id:` 是行号、认 `Last-Event-ID`）、`/api/tree?path=`（只列一层）、`/api/health`、`/api/questions/<id>`（只从事件流读「现在在等哪些表态」）。**写**只有三个文件：`data/ui/req/<ts>.txt`（`POST /api/run` 写一行需求 → `subprocess` 起对应模式的 agent 脚本（多窗口带 `--plain --force`））、`data/ui/inbox/<run_id>.jsonl`（`POST /api/answer` 一行一条裁决）、`data/ui/logs/<ts>.log`（子进程的人类可读日志）。装配预览 `POST /api/preview` 仍是**纯函数**。`prompt_builder.build()` 六条规则（原句一字不动 / 附件各成一句 `。参考X：token` / 全篇 `/Game` 最多一条 / 源码与文档只收相对路径 / 外部文件不进 prompt）由 `eval/check_entry_paths.py` 钉住。`agent/ui_ask.py` 是决策通道：`question` 事件发在拿答案**之前**、按钮文案与 `_pause_hint` 同源、收件箱按 `run_id`+`question_id` 双键「领取即改名」（`os.replace`）、**缺行不许当成自动 skip**（等不到就按停止收口）——`eval/check_entry_questions.py` 与 `eval/check_entry_drive.py` 钉住。这一层只 import `config/core/server`，不 import `agent`；`run_windows --ui-inbox` 与 `run_multi --ui-inbox` 由 `entry.py` 透传 `UE_RAG_UI_INBOX` |
| 任务 | `agent/task_schema.py`、`agent/run_task.py`、`agent/run_windows.py`、`agent/board.py`、`agent/brief.py`、`agent/ledger.py` | `--task` 跑固定 JSON。`--query` 仍是单循环。菜单 3 每扇一个新 `thread_id`（续接的中断扇用回原 id）。退出码 0/1/130。面板、任务书、事件流、机读 run 状态、快照、矩阵、台账都在这条线 |
| 评测 | `eval/run_eval.py`、`eval/report.py`、`eval/check_routes.py`、`eval/check_info_layer.py`、`eval/check_architecture.py`、`eval/check_schema.py`、`eval/check_risk.py`、`eval/check_automation.py`、`eval/check_scene_gate.py` | 默认仍是三条任务。`report.py` 按任务合计 token，没有 usage 就留空；对目录再写 P50/P95 和一页 HTML，并把记分板（tasks / ok / first_pass / prompt_tokens_per_ok / tool_refused）写进 `data/harness/scoreboard.json`。`check_routes.py` 检查图的边，也检查四个 HTTP 路由。`check_scene_gate.py` 钉住场景内容闸门：句子里点名的金币/平台/门必须补出 `place_batch` 与 `editor_assert`。trace 在 `data/traces/`，已 gitignore |

### 图怎么走

```text
retrieve → plan → 有工具调用 → execute → plan
                 → 无工具调用且 verify 非空 → build →（通过）→ test → observe
                                                      ↘ 失败 → observe → fix → plan
                 → 无 verify → 必须调用 finish 才结束
```

`verify` 为空时，`retrieve` 才把 `finish` 放进工具列表。有 `verify` 时模型不能自己宣布成功。`judge_first: true` 的任务第一轮不调用模型，直接进裁判。只编译工程的任务同时带 `freeze_source`：裁判失败就结束，不把错误送进 `fix`，因此不会改源码。修编译错误的任务不带这个标记，仍然先看到失败再改。

重试上限来自任务的 `max_retries`，缺省 3。失败才加一。`fix` 把 `errors` 写成下一条 user 消息，清空两份裁判，迫使下一轮重新编译或断言。

build 前打转分档（2026-10-14 补）：编译还没跑过时，写码成功后连续 8 次只在 `read_source` 的，不判失败，`plan` 直接返回空工具调用，借现有路由进 `build`——写好的代码不会被快照还原掉；最近 12 次调用全是读/diff 且失败 ≥6 次且期间没有一次写码成功的，才把图干净结束，诊断写进 `last_error`。`apply_unified_diff` 的冲突报错会指出对不上的那一行和文件里的实际内容；`read_source` 一次最多 200 行；trace 和事件流的 `tool_call` 行带工具失败原文。

当前检索是三层里的前两层。专有名词（类名、`/Game/`、`error C`、`BP_`）能唯一命中就停止。否则 BM25 与 bge 各取 20 条，用 RRF 合并。短名单重打分在原 22 题和扩到 30 题上都不高于路由加 BM25 的 Recall@5（四列都量过，重打分与混合都是 1.000），热路径不加。对照和没命中的题号在 `RAG/ue-rag/eval/reports/20261016-154827-retrieval.md`。BM25-only 单独一列只出现在这份评测里，日常检索不走它。两个向量模型收成 `rag_core._get_model` 一个。工具检索走同一漏斗的 `ue_tools`，卡片仍然不带 `input_schema`。技能卡片仍是 JSON，带 `triggers`、`tools`、`version`、`conflicts`，正文不进卡片。路由选出域之后只在该域的几张里挑最近的一张，plan 仍只带这一张的说明。选出域时再附上引擎 AgentSkill 的 `GeneratePrompt` 结果，最长 1200 字；没选出域就不附。

`fix` 会先按错误原文检索一条 `source_type=ue_error`，没有命中就不编造历史。

`plan` 发给模型的列表按角色切开：系统提示、原始用户句、最近一条「裁判失败」、当前角色允许的最近 6 条工具结果。最近一条成功的 `apply_unified_diff` 不受这 6 条排挤，正文保留。更早的工具正文在写入 checkpoint 时换成「工具名 + sha1 + 可再读」，工具名和 `tool_call_id` 还在。这次切片只把 `user`、`judge_fail`、`tool_kept`、`tool_dropped` 四个计数写进事件 `context_slice`。`cpp` 角色的请求里不会出现 `set_actor_transform` 的工具结果。图的边没有改。没有为了压缩再打一次模型。

### 执行器

`exec_tool(toolset, tool, args, task)` 返回完整 `{ok, data|error}`，不在这里截断。`call_mcp_tool.py` 的 stdout 仍截到约 200 字。

拒绝规则：

- `save_assets`、`save_actor` 需要任务 `save` 为真。任务没写 `save` 时跟随 `UE_DEFAULT_SAVE`（缺省 `true`）；显式写 `save: false` 才拒绝。CLI 要加 `--save`。
- `StartPIE`、`StopPIE` 需要任务 `pie` 为真。任务没写 `pie` 时跟随 `UE_DEFAULT_PIE`（缺省 `true`）；显式写 `pie: false` 才拒绝。CLI 要加 `--pie`。
- 短名命中 `Set` / `set_` / `Spawn` / `Delete` / `Add` / `Remove` / `Create` 等写前缀时，必须出现在 `allow_writes` 里，否则 `{ok:false, error:"write refused"}`。`query_actor` 和 `run_automation_test` 不是写前缀，不在 `allow_writes` 里也能读。
- 拒绝结果带 `policy`：当时的 `save`、`pie`、`allow_writes`，以及 `UE_ALLOW_BLUEPRINT`、`UE_ALLOW_MATERIAL`、`UE_ALLOW_UMG`、`UE_ALLOW_ANIM`、`UE_ALLOW_NIAGARA` 的当前值。mock 模式也先过这些拒绝。
- 同一 MCP 短名连续失败 5 次触发熔断（2026-10-20 约束层，`core/fault.py`）：写 `data/harness/circuit.json`，冷却 30 秒，期间 `core/risk.py` 的 `decide` 拒绝，`error` 是 `circuit open`。熔断拒绝不改 `write refused` 的语义，`policy` 键不变。路径可被 `UE_CIRCUIT_PATH` 覆盖。
- 读工具不拦。
- CLI 没有任务文件时，`allow_writes` 只有这一次的 `--tool`。

`UE_EXEC_MODE` 默认 `real`。只有显式设成 `mock` 才走两个假 Actor，此时裁判也返回假结果。`real` 下 `INJECT_*` 不生效。

普通 MCP 调用超时 60 秒（`UE_MCP_CALL_TIMEOUT`）。裁判和编译等待 300 秒（`UE_MCP_TIMEOUT`）。

`cpp_build` 开始时把进程号、开始时间和截止时间写进 `data/cpp_build_job.json`。截止沿用 `UE_MCP_TIMEOUT`（300 秒）。到点，或人按下停止，就结束这个进程树，裁判记失败，不把超时写成编译成功。停止是这次编译还在跑时的 Ctrl+C，或在 `RAG/ue-rag` 另开终端执行 `python -c "from core.build_job import request_stop; request_stop()"`。没有第二份任务队列。

### 裁判

下表的工具名是 `backend=official`。`backend=monolith` 时位置、编译和 Automation 的分叉见「当前系统」。

| kind | 做什么 | 怎样算通过 |
| --- | --- | --- |
| `editor_assert` | 读任务指定的工具。Actor 可先 `find_actors`，命中不是 1 个就停下并列出候选 | `expect.location` 的 x/y/z 与结果相差不超过 1 uu |
| `blueprint_compile` | `BlueprintTools.compile_blueprint`，参数 `{blueprint:{refPath}}` | 客户端 `ok` 且 data 为 null 或空 |
| `cpp_build` | 先关编辑器，再 `Build.bat AIAgentProjectEditor Win64 Development -Project=... -WaitMutex`。开始时写 `data/cpp_build_job.json`。超时或停止会结束进程树 | 退出码 0，且日志里没有 `error C` / `fatal error`。最多抽取 20 条。编的是编辑器要加载的模块。超时和停止记失败，不看被杀掉之后的退出码 |
| `automation` | 同进程内 `DiscoverTests` 一次，再 `RunTestsByFilter`，再 `GetTestResults` | JSON 里的 `failed` 为 0。解析不到失败数就是失败。单词 `passed` 不算成功 |
| `skip` | 这一侧没有声明 | `ok: true`。observe 把 skip 当成通过 |

5.8 没有可用的编译 commandlet。`UnrealEditor-Cmd.exe -run=Compile` 此前已失败，不再当候选。自动编译在关掉编辑器之后编 `AIAgentProjectEditor`。游戏目标 `AIAgentProject` 的 DLL 编辑器不加载，新的 `UPROPERTY` 不会出现在已经启动的编辑器里。Cmd 继续只负责无界面导出。编译通过并且这次留下了源码修改时，跑 `shortcuts\cpp.bat nopause` 重新导出 C++ 并入库，再启动 `UnrealEditor.exe`。编辑器工具若因连接被拒绝失败，会先尝试启动编辑器并等待 MCP 最多 120 秒，然后重试这一次调用。等不到就不编造成功。若这次任务还要编译，编译前会再次关掉编辑器。

编辑器开着且 Live Coding 启用时，不要编 `AIAgentProjectEditor`，所以编译前必须先关掉编辑器。过滤器没有命中任何测试时，`RunTestsByFilter` 报错，不是 `failed: 0`。单词 `passed` 不算成功。任务带了 `automation_filter` 且 `cpp_build` 已通过时，测试节点会跑这条 Automation。编译失败和 `editor_assert` 失败仍回到 `fix`。编译已过、Automation 仍失败时，本扇结束（`is_completed` 为真、`final_success` 为假），不进 `fix`；父级按失败测试名另开处置扇 `dispose_residual`（只由代码追加，规划者写不出，一簇一扇，最多 4 扇，多出来的并进 key 为 `overflow` 的第 4 扇）。错误里带失败计数和测试名。残差要被后续同 kind 裁判标成 `passed: true`，或在 `data/handoffs/<run_id>/<window_id>.attribution.json` 里标成 `out_of_contract` / `equivalent`，处置扇才算成功。`blind_spot` 只表示还要补测试，不能单独结案。归因说明不把条款改成 `passed: true`。三种标签只此三个词。

`build_node` 跑 `cpp_build` 和 `blueprint_compile`。`test_node` 跑 `editor_assert` 和 `automation`。两边都要 `ok` 才算任务成功。

### 本地补丁

C++ 不让模型直接写文件。

- `read_source(path, start_line, end_line)`：相对 `Source/AIAgentProject`，拒绝 `..`，一次最多 200 行。
- `apply_unified_diff(path, diff)`：同一路径。只接受 unified diff。旧行必须与文件一致，否则返回冲突且不写入。

任务成功、并且 diff 写过 `Source/`、又不是试探任务时，`run_task.py` 调用 `shortcuts\cpp.bat nopause` 重新导出并入库。同一轮如果重试后成功，并且 `documents/历史解决方案.md` 里还没有同名标题，就追加一节：标题是错误原文的第一句，正文只有现象和用过的工具短名。然后跑 `export_docs.py`。只有历史、没有 C++ 改动时，再单独跑一次 `ingest_assets.py`。失败任务不入库，也不编造改法。写过 `Source/` 且会留下修改时，再追加当天 `.cursor/memory/任务记录/YYYY-MM-DD.md`，并镜像到 `.codex/memory/任务记录/YYYY-MM-DD.md`。`inject_typo` 且会还原的任务不写任务记录，也不重新导出。

`inject_typo` 的任务在 `finally` 里把文件字节还原，不把试探错误留在工作区。两个仓库都有 git。关卡和 `Content/` 不入库。计入成功率的清单在 `RAG/ue-rag/eval/bench_manifest.json`。手工 Cursor 对照列仍是空的。旁路说明在 `RAG/ue-rag/eval/tasks/side/README.md`。默认三条不变。单点杀伤 `eval/tasks/side/kill-check.json` 不进默认三条，也不进 `bench_manifest.json`。它仿 `inject_typo`：对已经通过的 `AIAgent.Pawn.Look` 故意改错一处，期望 Automation 失败，`finally` 按同一条源码快照还原。

## 回滚

源码用 `data/snapshots/<run_id>/<window_id>/` 的字节快照还原。编辑器侧的写前快照在 `agent/prop_snapshot.py`：scalar 记旧值并写回；`place_batch` 新建的 Actor 按名登记，恢复时只删唯一实例；`set_pickable` 和 `move_actor` 把写入前的 `bPickable`、`Tags` 或位置记在 `data/snapshots/<run_id>/props.json`。每条记录带动作、目标 ref、逆操作、幂等键，方式是 `compensating`。这一扇失败且任务没有 `keep_source` 时写回，写回后按任务策略保存关卡（缺省保存）。同一幂等键只回滚一次；同一 `ref` 旧值不一致则都不做，并写 `manual_intervention`。新建名单里同一实例记两次也只删一次。编译失败不消费快照；保存失败把关卡差异留在快照目录。逆操作连接失败就停，不再做下一条。被改判 orphaned 的 run，续接前先按快照逆序恢复。索引里仍然没有 Transaction / Undo 工具，这些快照不代替编辑器事务。`editor-transaction` 因此仍是 pending。

## 记忆

三类，没有人格。文件、谁写入、保留多久写在 [`documents/记忆.md`](documents/记忆.md)。

- 短期：checkpoint、`data/traces`、`data/events`、`data/handoffs`、当轮任务书、验收合同和快照。给模型的是原始句、最近一条裁判失败、当前角色最近 6 条工具结果，外加最近一条成功 diff。上一扇只以交接 JSON 进入下一扇。
- 长期项目：`conventions.md`、`project_context.json`、`skill_cards.json`、`learned_skills.json`（已提升，最多 200 条）、`learned_routes.json`（兼容，最多 50 条）、`failure_patterns.json`、`clarify_policy.json`、`ledger.json`、当天任务记录。影子技能和教师候选不进 L0。
- 错误：成功重试才写 `ue_errors.jsonl` 和 `documents/历史解决方案.md`。`failure_patterns.json` 只收至少两次裁判成功、且症状对得上的修复。蒸馏里的失败样本保留 `error Cxxxx`，最长 240 字，路径和密钥会脱敏。排障条目由人写。失败任务不入库。`compare_actions` 只比动作名，报告里写明它不是结果回放。

失败行的 `failure_class` 是 `compile`、`test`、`tool_refused`、`mcp_down`、`retry_exceeded`、`level_precheck`、`other` 之一，写在 trace 里。报告仍印中文。枚举在 `RAG/ue-rag/core/failure_class.py`。

### 检索里有什么

| source_type | 来源 | 谁用 |
| --- | --- | --- |
| `ue_asset` / `ue_actor` | `export_asset_metadata.py`，要关编辑器 | `search_ue_project` |
| `ue_cpp` | `export_cpp_metadata.py` 扫 `Source/AIAgentProject`。`UCLASS(...)` 括号配平，和 `class` 之间可以换行，同一文件多个类各一条 | 同上 |
| `ue_bp_graph` | 编辑器开着时 `export_blueprint_graph.py` 只读 `list_graphs`、`list_variables`、`list_events`。只留路径、图名、变量名、事件名。2026-10-13 入库 4 条 | 同上，可 `where={"source_type":"ue_bp_graph"}` |
| `ue_doc` | `export_docs.py` 切 `documents/*.md`（含 `documents/harness/*.md`，2026-10-20 起）和 `.cursor/memory/conventions.md` 的二级标题 | 同上，可 `where={"source_type":"ue_doc"}` |
| `ue_error` | 成功重试后的错误原文 sha1 | `fix` 先查一条 |
| 技能卡片 | `export_skill_cards.py` 先读 `.codex/skills/*/SKILL.md`，同名再跳过 `.cursor/skills`。只取前 20 行的 name 和 description，再补 `triggers`、`tools`、`version`、`conflicts`。正文不进卡片。蓝图改图遇到 `UE_ALLOW_BLUEPRINT=false` 时，冲突产出是 `Plan/` 里的人工步骤。磁盘上当前 24 份 `SKILL.md`，含 `ue-monolith-backend-skill`；入库条数以 `export_skill_cards.py` 的产出为准 | 不进向量库。plan 时只带一张卡片的说明。选出域后附 `data/agent_skill.json` 里的生成结果，最长 1200 字 |
| 排障条目 | `documents/历史解决方案.md`（`doc_kind=history`）、`documents/排障条目.md`（`doc_kind=debug`） | 仍是 `ue_doc`。一次只取 `top_k=1`，标题对上才用 |

`Plan/` 不入库。蓝图索引导出的是图名、变量名和事件名，不是整段 Graph DSL，也不等于改图。材质、UMG、动画蓝图、Niagara 不导出图。编辑器没开时这一层跳过，不编节点名。

`search_ue_project` 的可选 `where` 传给 Chroma。不传时和原来一样。排障用 `{"doc_kind":"history"}` 或 `{"doc_kind":"debug"}`。

### 排障时进上下文的内容

历史方案和 debug 症状会变长，所以不读整份 Skill，也不读整份 markdown。

稳定留在上下文里的只有技能卡片上的 description，外加规则里的这几步：

1. `search_ue_project(query=现象原文, top_k=1, where={"doc_kind":"history"})`
2. 返回的 `metadata.title` 与现象对不上，再查 `where={"doc_kind":"debug"}`，仍是 `top_k=1`
3. 仍对不上，就当没有现成方案，走域技能，并且只读用到的那一节

新的历史现象写成 `documents/历史解决方案.md` 的一个二级标题。新的 debug 症状写成 `documents/排障条目.md` 的一个二级标题。然后跑 `export_docs.py` 和 `ingest_assets.py`。不要把案例追加进 `SKILL.md`。闭环的 `plan` 同样只在标题对上时附上那一节，最长约 1200 字。

## 仓库怎么放

```text
E:\UEProject\AIWork\
├── AIAgentProject\          UE 5.8 工程 + Cursor 规则
│   ├── AGENTS.md            Token 节流与工具路由
│   ├── README.md            项目说明
│   ├── 周计划.md            13 周学习路线
│   ├── WORKFLOW.md          本文件
│   ├── .cursor\             Cursor 规则、Skill 镜像、人读记忆
│   ├── .codex\              技能正文、Codex 短文与钩子
│   ├── .agents\skills\      Codex 技能发现目录，联接到 .codex\skills
│   ├── documents\           功能设计。导出后以 ue_doc 进入检索
│   ├── start.bat            唯一入口：双击起 server/entry.py（8011），服务自己开浏览器
│   └── Source\              C++。本地 diff 只能改 AIAgentProject 这一棵
└── RAG\ue-rag\
    ├── config.py            路径和超时的唯一缺省
    ├── .env                 密钥和本机覆盖。不提交
    ├── backends\            一次运行锁一个后端：select / official / monolith
    ├── core\                官方执行器、裁判、检索、MCP 客户端、diff、风险与熔断
    ├── agent\               图、节点、run_windows.py、diagnose.py、sequence.py、catalog.py、harness.py
    ├── eval\                任务、run_eval.py、report.py、离线检查
    ├── pipelines\           导出与入库
    ├── scripts\             官方扫描、单次调用、monolith_backend.py
    ├── server\              检索 MCP、六工具契约、DSH 网关、四个 HTTP 路由（8010，不跑图）、HTML 入口层（8011，投影 + 装配预览 + 请求文件/收件箱驱动）
    └── data\                JSONL、技能卡片、Chroma、traces、costs.jsonl、harness\（feature_list、progress、risk_matrix、arch_whitelist、scoreboard、garden_report、circuit）
```

| 目录 | 谁读 | 干什么 |
| --- | --- | --- |
| `AGENTS.md` | Cursor 与 Codex | 铁律和路由。`.codex/AGENTS.md` 只是指针，仓库根开工时 Codex 不加载它 |
| `.cursor/rules/*.mdc` | Cursor，始终生效 | 动编辑器先看 `UE_EDITOR_BACKEND`：`official` 先扫描工具，`monolith` 走 discover / describe / exec。改 UE 项目先计划、后记录 |
| `.codex/instructions/` | Codex，`SessionStart` 注入 | 与 `.cursor/rules` 对等的短文 |
| `.cursor/skills/` | Cursor 镜像；卡片在同名时不读 | 域操作说明，含计划、路由、人工蓝图、历史 |
| `.codex/skills/` | 技能正文的源 | 改完同步到 `.cursor/skills`。Codex 经 `.agents/skills` 发现 |
| `.codex/hooks.json` | Codex，`/hooks` 信任后 | 注入短文，并在 shell 里拦写图 |
| `.cursor/memory/` | 两边的人读记录 | 计划、约定、任务记录。约定会进检索 |
| `.codex/memory/任务记录/` | 闭环镜像 | 成功任务的追加副本。不另写计划或约定 |
| `documents/` | 人，以及导出后的检索 | 功能设计。实现某个系统时仍当需求说明读 |
| `RAG/ue-rag` | 检索、执行器、闭环命令 | 语料、工具卡片、裁判、评测 |

## 拉取之后先准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`，启用官方 Unreal MCP，确认 `http://127.0.0.1:8000/mcp` 能连。编辑器没开时，写场景和改蓝图都会失败。
2. 用 Python 3.12 在 `RAG/ue-rag` 建虚拟环境。现成环境是 `.venv`（3.12.10）。依赖见 `requirements.txt`，其中包含 `openai`、`mcp`、`langgraph`、`langgraph-checkpoint-sqlite`。
3. 复制 `.env.example` 为 `.env`。至少填 `DEEPSEEK_API_KEY`。`UE_LLM_MODEL=` 留空就是 `deepseek-chat`。密钥只放这里。`pyvenv.cfg` 里出现过的旧密钥视为已经泄露，到服务商处作废。
4. 路径要改时，改 `.env` 里的 `UE_EDITOR_CMD`、`UE_PROJECT`、`UE_SOURCE_ROOT`、`UE_MCP_URL`。空着就用 `config.py` 的本机缺省。
5. 把项目语料 MCP 配进 Cursor（stdio，由客户端拉起）：

```json
{
  "mcpServers": {
    "mcp_ue_rag": {
      "command": "E:\\UEProject\\AIWork\\RAG\\ue-rag\\.venv\\Scripts\\python.exe",
      "args": ["E:\\UEProject\\AIWork\\RAG\\ue-rag\\server\\server.py"]
    }
  }
}
```

仓库里没有提交这份配置。工具是 `search_ue_project`、`get_project_summary`。`search_ue_project` 可以多传一个 `where`。

六个高层工具在旁边的 `RAG/ue-rag/server/contract_server.py`。`tools/list` 只有契约 `RAG/ue-rag/contract/tools.json` 里的六个，不返回官方 830 个。其中 `query_actor` 和 `run_automation_test` 也登记进规划者的只读动作表，另外四个不进。读场景的 `query_actor` 另在工程 `Content/Python/aiagent_query_actor_toolset.py` 注册成 Python Toolset，schema 由 `get_toolset_json_schema` 导出到 `RAG/ue-rag/contract/query_actor.toolset_schema.json`。编辑器下次启动时 `Content/Python/init_unreal.py` 会再注册一次。项目规矩另有一条 AgentSkill，资产在 `/Game/Skills/AIAgentProjectRules`，说明是 1 米 = 100 uu、编 `AIAgentProjectEditor`、路径不唯一就停、裁判不由模型宣布。`GetSkills` 带回的 instructions 写在 `RAG/ue-rag/data/agent_skill.json`。`project_context.json` 只有模块名、五个开关、模型名、源码根和当前关卡名。

6. 第一次使用前入库。Embedding 是 `BAAI/bge-small-zh-v1.5`，镜像 `https://hf-mirror.com`。

```bat
shortcuts\cpp.bat       C++ → ingest。改过并留下的 C++ 由闭环自动调用，参数 nopause。双击则跑完暂停
shortcuts\asset.bat     无界面编辑器导出资产和 Actor。本工程编辑器开着会失败
shortcuts\mcp.bat       重建工具索引并写入 ue_tools
```

文档和技能卡片另跑：

```powershell
python pipelines/export_docs.py
python pipelines/export_skill_cards.py
python pipelines/ingest_assets.py
```

编辑器开着、而且不要保存、不要 PIE 时，可以再跑只读刷新。连不上就停，不编资产名或节点名：

```powershell
python pipelines/export_blueprint_graph.py
python pipelines/refresh_editor_readonly.py
```

`export_blueprint_graph.py` 写入 `data/ue_bp_graph.jsonl`。`refresh_editor_readonly.py` 不调用 `UnrealEditor-Cmd`，也不加载其他关卡。全量关卡导出仍用 `asset.bat`，而且要先关编辑器。

`asset.bat` 和 `mcp.bat` 使用的编辑器路径来自 `config.py`，不再在 bat 里写第二份绝对路径。

## 日常对话怎么走

在 Cursor 或 Codex 里用中文或英文说需求。Cursor 始终生效的是 `.cursor/rules`。Codex 读根目录 `AGENTS.md`，短文由 `.codex/hooks.json` 注入；钩子未信任时先读 `.codex/instructions/`。这条链不调用 `run_task.py`。第一次在 Codex 里执行 `/hooks` 信任钩子，并跑 `.codex/link_agents_skills.ps1`，技能才会从 `.agents/skills` 被发现。

### 只问项目里有什么

没有确切资产路径或类名时，第一动作是 `search_ue_project(query, top_k=5)`。返回 `id`、`content`、`metadata`、`distance`。`metadata.source_type` 可以是 `ue_asset`、`ue_actor`、`ue_cpp`、`ue_doc`、`ue_error`。

这次调用只查向量库。结果空了或不相关，再退回读源文件。有确切路径或要读实时属性时，改走编辑器工具。

`get_project_summary()` 返回 `ue_assets` 的条数。

### 要改编辑器

先看 `RAG/ue-rag/.env` 的 `UE_EDITOR_BACKEND`。未写或 `official` 时，移动 Actor、读属性、编译已有蓝图走下面的官方扫描，读 `ue-rag-route-skill`。`monolith` 时不扫描 830 个工具，读 `ue-monolith-backend-skill`，在 `RAG/ue-rag` 用该目录的 Python：

```powershell
python scripts/monolith_backend.py --discover
python scripts/monolith_backend.py --describe --namespace mesh --action get_actor_info
python scripts/monolith_backend.py --exec --namespace mesh --action get_actor_info --args "{\"actor_name\":\"Cube\"}" --no-save --no-pie
```

`official` 才用：

```powershell
python scripts/scan_mcp_tools.py --query "把 Cube 上移 100" --top-k 8
python scripts/scan_mcp_tools.py --schema "<full_name>"
python scripts/call_mcp_tool.py --toolset "<toolset>" --tool "<短名>" --args "{...}"
```

官方扫描只返回卡片，没有 `input_schema`。`--tool` 填短名。`call_mcp_tool.py` 在 `backend=monolith` 时直接拒绝。stdout 约 200 字。

没写 `--save` / `--pie` 时跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`（缺省 true）。要关掉加 `--no-save` / `--no-pie`。同一件编辑器任务最多 8 次调用。只验证点名的那一项。扫描或 discover 两次仍对不上就停，不要把 schema 拉进对话。选错入口就停，不改连另一边。

域 Skill 在选中工具之后再读，一次只读用到的那几个。正文以 `.codex/skills` 为源，改完同步到 `.cursor/skills`。新踩的坑追加到 `ue_mcp_verified.json`，不要整文件读入。索引 `ue_mcp_index.json` 约 1.4MB，同样不要整文件读入。Monolith 的域卡片在 `ue-monolith-backend-skill/cards/`，一次只读一张。

官方 MCP 索引版本记在事实库：2026-09-21，引擎 5.8.2，52 个工具集、830 个工具。这条只描述 `official`。

域 Skill 在选中工具之后再读，一次只读用到的那几个。正文以 `.codex/skills` 为源，改完同步到 `.cursor/skills`。新踩的坑追加到 `ue_mcp_verified.json`，不要整文件读入。索引 `ue_mcp_index.json` 约 1.4MB，同样不要整文件读入。

官方 MCP 索引版本记在事实库：2026-09-21，引擎 5.8.2，52 个工具集、830 个工具。

### 要改 C++，并且会落到工程里

先读 `.codex/skills/ue-cpp-plan-skill/SKILL.md`。分层见 `patterns.md`：血量、背包、AI 放 Component、Subsystem 或 Controller。

1. 锁定一个同模块的现成类。没有路径就先 `search_ue_project`。
2. 即将改工程时，把六段计划追加到 `.cursor/memory/plans/YYYY-MM-DD.md`，等确认。
3. 确认前不写 `.h` / `.cpp` / `Build.cs`。
4. 确认后只做这一件事。表现留给 `BlueprintImplementableEvent`。
5. 人点名才验证一次。
6. 工程真的改完之后，追加 `.cursor/memory/任务记录/YYYY-MM-DD.md`。可复用的约定再写入 `conventions.md`。

### 要改图

先看 `UE_EDITOR_BACKEND`，再看 `UE_ALLOW_BLUEPRINT`。`monolith` 时不扫描官方工具，写许可仍看对应的 `UE_ALLOW_*`；不是 `true` 就把步骤写入 `Plan/` 然后停。开关为 true 时，闭环对同一张蓝图最多修 3 次，见「当前系统」。`official` 且 `UE_ALLOW_BLUEPRINT=false` 时蓝图不改，按 `.codex/skills/ue-manual-blueprint-skill` 把连线步骤写入 `Plan/`。`true` 时走 `ue-blueprint-skill`，失败次数到 `max_retries` 就停。上面的 hook 会在 Cursor 的 shell 和 Codex 的 `PreToolUse` 里拦住同一批写图工具。材质图、UMG、动画蓝图、Niagara 在对应的 `UE_ALLOW_*` 不是 `true` 时只给人工步骤。只编译一张已有蓝图走 `blueprint_compile`，不改图。`monolith` 时编译走 `blueprint.compile_blueprint` 或 `ui.compile_widget`，成功看 `success` 与 `error_count`，不保存。

单行 bug 或明确说「直接改」：可以不写大计划，动了 UE 工程仍要补当天任务记录。改 Cursor 规则、改 Skill、只分析文档、只问答：不写计划，也不写任务记录。

已沉淀的约定：1 米 = 100 uu；速度用 uu/s。键盘字母用 `BindKey`，本引擎只有 4 参数成员函数指针重载。C++ 导出按配平后的 `UCLASS(...)` 认类，和 `class` 之间可以换行。Actor 坐标是导出当时的快照，检索正文里写「坐标不是实时位置」。

人点名的证据仍是 `ue-verify-skill`。`official` 时位置用 `get_actor_transform`，蓝图编译成功看返回 null。`monolith` 时位置用 `mesh.get_actor_info` 的 `transform.location`，编译成功看 `success` 与 `error_count`。日志用 `GetLogEntries`。玩法类验收要 PIE 时按 `UE_DEFAULT_PIE` 走，缺省开。

## 命令闭环怎么跑

工作目录 `RAG/ue-rag`，Python 用 `.venv`。

一份任务：

```json
{
  "id": "bp-gun-compile",
  "query": "编译 BP_Gun 并确认没有编译错误",
  "verify": [{"kind": "blueprint_compile", "refPath": "/Game/BP_Gun.BP_Gun"}],
  "allow_writes": [],
  "save": false,
  "pie": false,
  "max_retries": 3
}
```

```powershell
python agent/run_task.py --task eval/tasks/bp-gun-compile.json
python agent/run_task.py --query "修掉 TestActor 的编译错误"
python eval/run_eval.py
python eval/report.py data/traces/eval-<timestamp>.jsonl
python eval/check_routes.py
python eval/check_upgrade.py
python eval/check_distill.py
python eval/check_harness.py
python eval/check_info_layer.py
python eval/check_architecture.py
python eval/check_schema.py
python eval/check_risk.py
python eval/check_automation.py
python eval/check_scene_gate.py
python agent/distill.py --dry-run
python eval/retrieval_bench.py
python eval/understand_probe.py
python eval/model_compare.py
python eval/report.py data/traces
python server/gateway.py
python server/entry.py --port 8011   # 会自己打开浏览器；--no-open 关掉
```

`report.py` 的列是 id、ok、retry、tools、elapsed、`prompt_tokens`、`completion_tokens`。token 按任务把每行里有的数加起来。没有 usage 就印空白，不估算，也不填 0。最后一行是 `tasks=` 和 `ok` 为 true 的个数。接着按失败行打分类计数：编译、测试、工具拒绝、MCP 连不上、超过重试、关卡预检、其他。trace 里已有 `failure_class` 时以这个枚举为准，没有字段的旧行仍按原文归类。对不上就进「其他」。参数如果是 `data/traces` 目录，每个 jsonl 一行：文件名、任务数、`ok` 数、两类 token 合计，然后再打一份合计分类，以及耗时 P50/P95、平均 token、重试合计。路由计数只读 `data/events` 里的 `run_start`：`rules` 和 `learned` 算 L0 直接归地，`understood` 是理解器，`planner` 是老规划者。同时在 `eval/reports/` 写一页静态 HTML，打开就是这些数。交接目录或 trace 里真有归因条目时，这一页多一节：id、簇 key、label、截短的 note；没有条目就不写这一节。缓存命中率只有 trace 里真有缓存字段时才填。不算钱，不另起采集服务。`observe` 行带 `judge_kind` 和截短的错误原文，分类靠这两项。`check_routes.py` 通过时打印 `routes ok`，退出码 0。它覆盖「编译失败或 `editor_assert` 失败回到 `fix`」（编译已过的 Automation 残差不进 `fix`，见上面的裁判），也覆盖 C++ 换行导出、快照标记、角色上下文、hook 拒绝、分类，以及四个 HTTP 路由和表外工具拒绝。`check_routes.py` 末尾会再跑 `check_upgrade.py`。九条离线检查一起通过时打印 `routes ok`、`upgrade ok`、`distill ok`、`harness ok`、`info ok`、`architecture ok`、`schema ok`、`risk ok`、`automation ok`（后五条是 2026-10-20 三轮升级加的，也进了 `.github/workflows/offline.yml`）。不要为了制造失败去改游戏源码。

`--query` 先经 `plan_sequence`。加函数或修编译错误时，写不出唯一的类就失败。编译工程不要求唯一类。拾取的两个槽各自解析，一个槽零条或多条才暂停。蓝图 `refPath` 和 Actor 对不上时不猜。`UE_ALLOW_BLUEPRINT=false` 时，改蓝图的一句话不进图，并在 `Plan/` 写下操作步骤。`true` 时改那张唯一的蓝图，编译失败重试到 `max_retries` 后停。评测仍用 `--task`，避免监督者把三条基线跑歪。`eval/tasks/cpp-fix-keep.json` 与 `cpp-typo` 相同，但 `keep_source: true` 时不还原源码；只有明确要留下修改时才跑。

`run_agent.py` 仍留在磁盘上，菜单不再调用。它是以前的手工对话，没有任务文件时写操作会被拒绝。规划留在 Cursor 的时候不要把它当日常入口，也不要把菜单 3 接成每句话的默认动作。

三条评测，2026-10-12 一次连跑都通过：

| id | 在验什么 | 那次结果 |
| --- | --- | --- |
| `editor-cube` | 把 `L_NewTest` 里名为 Cube 的 Actor 移到 `(1200, 400, 150)`，再读位置。`save: false` | 通过，0 次重试。关卡未保存，重载关卡即回到原位 |
| `bp-gun-compile` | 只编译 `/Game/BP_Gun.BP_Gun` | 通过。不要求改图 |
| `cpp-typo` | 向 `TestActor.cpp` 写入一个不存在的调用，裁判先失败，再 `apply_unified_diff` 删掉它，游戏目标编译通过，最后还原文件 | 通过，1 次重试，trace 里有 `apply_unified_diff` 和 `error C3861` |

`editor-cube` 要求当前关卡名包含 `L_NewTest`。编辑器没开时，这一次调用会先尝试启动编辑器并等待 MCP。关卡名对不上就停，不切换关卡，不保存。`run_eval.py` 在这种情况下把这一条记失败，另外两条继续。

`automation` 裁判已经接上，默认三条里没有它。`DiscoverTests` 仍只返回 `{"status":"ready"}`。过滤器名从 `ListTests` 抄：`AIAgent.Pawn.Look`、`AIAgent.Pawn.Wasd`、`AIAgent.Enemy.Patrol`。巡逻任务是 `eval/tasks/side/demo-c-patrol.json`，不进默认三条。

Checkpoint 在 `RAG/ue-rag/agent_checkpoints.db`。每次任务用新的 `thread_id`。

## DeepSeek Harness（可选外壳，2026-10-23 起第二入口）

DSH 只做外壳：对话、研究、只读并行、任务书。检索、执行、裁判、回滚仍是 `RAG/ue-rag` 内核。口径以根目录 `AGENTS.md` 末尾「DeepSeek Harness（2026-10-23 起，第二入口）」一节为准，这里只写怎么用。

入口：DSH 打开工作区 `E:\UEProject\AIWork`，配置模板 `RAG/ue-rag/dsh/cordis.patch.yml`，`/permission` 设 `workspace-write`。开工先 `resume_context`，不读本文全文。

工具面只有 9 个 `mcp__uerag__*`，一个工具一行：

| 工具 | 作用 |
| --- | --- |
| `search_project` | 语义检索项目语料（资产 / C++ / 蓝图 / 文档 / 历史错误） |
| `scan_editor_tools` | 按自然语言找编辑器工具，只出卡片。`monolith` 时改为 discover |
| `get_editor_tool_schema` | 只取一个工具的 schema。`monolith` 时是一次 describe |
| `exec_editor_tool` | `official` 调 `editor_exec.exec_tool`；`monolith` 调 `backends/monolith.py` 的 exec |
| `read_contract` | 读 run 的 criteria / passed |
| `verify` | 跑裁判，`judge.ok` 且 kind 匹配才把 passed 改 true |
| `read_handoff` | 读扇间交接 JSON |
| `window_state` | 源码快照 / 还原，不保存关卡 |
| `resume_context` | goal / phase / pending / 最新 unmet |

写策略：`allow_writes` / `save` / `pie` 只从 `RAG/ue-rag/data/dsh/session_task.json` 读，该文件只由人写。

- 模型参数里没有这三个字段，网关只读文件；文件缺失即 fail-closed，所有写操作拒绝。
- 拒绝结果带 `policy`（当时的 save / pie / `allow_writes` 与五个 `UE_ALLOW_*`）。

校验（离线，工作目录 `RAG/ue-rag`，Python 用该目录 `.venv`）：

```powershell
python eval/check_dsh_config.py
python eval/check_dsh_bridge.py
```

`check_dsh_contract.py`、`check_dsh_lock.py`、`check_dsh_costs.py` 只依赖本仓库（临时目录 + mock / 假 judge），已进 `.github/workflows/offline.yml`。上面两条不进 CI：`check_dsh_config.py` 要同级 `AIAgentProject` 仓库与绝对路径的 `.venv`，`check_dsh_bridge.py` 要拉检索模型（首次约 1–3 分钟，180s 超时重跑一次即可），都只在本机跑。

禁止：

- 不接官方 UE MCP 的 830 个工具：DSH 会把 server 全部工具 schema 注入每个请求；也不在 DSH 里拉全量 schema。
- 模型文本「已完成」不改 `passed`；`manual` 条款只由人在 `Plan/` 里绑。
- 不用 DSH 的 read/write 直接改 `.uasset` / `.umap` / `Source/`，UE 写只经 `mcp__uerag__*` 网关。
- DSH 会话不写 `allow_writes`、不写 `save/pie`、不写 `.env`。

安装、真机验证三步、隐私与排障见 `RAG/ue-rag/dsh/README.md`。

## 仍然没有的东西

机器可读副本是 [`RAG/ue-rag/data/harness/feature_list.json`](../RAG/ue-rag/data/harness/feature_list.json)，`status` 冲突时以 JSON 为准。

这些挡在「任意一句话生成一个会巡逻的敌人」前面的，不是 C++ 半边，也不挡已经跑通的窄任务：

- `patrol-presentation` 仍是第一条 pending。C++ 状态机已编译，`AIAgent.Enemy.Patrol` 自动化已通过。`OnPatrol` / `OnChase` / `OnMeleeAttack` 已从 `BlueprintImplementableEvent` 改成组件上的 `BlueprintAssignable` 多播委托、`BP_TestEnemy` 的 EventGraph 已接上，剩余是跑出一条带 `evidence_sha1` 的 run 记录。`.env.example` 里 `UE_ALLOW_AI` 已经是 true，这条仍不因此改成 passed。自动条款过了、人工条款还在，整句是「部分完成」。步骤记录见 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`
- `backend=monolith` 且对应写开关为 true 时，蓝图类动作最多试 3 次，不保证成功。第 3 次失败就停，不改连 8000。开关不是 true 时次数是 0，步骤写入 `Plan/`。表现接线的三个事件不在这 3 次里
- 平台跳跃的 `material-domain`、`umg-domain`、`platform-automation`、`level-assembly`、`umg-hud-exec`、`level-assembly-persist` 在 `feature_list.json` 里已是 done。整句证据是 `data/runs/20261027/20261027-191740.md`。`check_platform_hud` 核对控件名和父类 `PlatformHUDWidget`。已存在实例的属性读写见下一条
- 交叉编码器重排、换 Embedding、Docker。观测页是 `report.py` 从已有 trace 写出的静态 HTML，不是常驻网站。`data/harness/scoreboard.json` 当前是 26 个任务、24 个 ok、每个成功约 126k prompt tokens；DSH 对照列仍空
- monolith 没有 `get_properties` / `set_properties`。`judge_pickup` 与 `set_pickable` 在覆盖矩阵里是 `partial`：裁决读不到实例属性，`bPickable` 写不上（标签可写）。已放进关卡的实例因此不能复读或重写 props；`place_batch` 在生成当时写 props 可以。其余 CATALOG 动作是 `both`。20261027-180955 里「场景动作直连 official 被拒」是修桥之前的失败，不是当前路径
- 20261027-180955 的合同假通过（窗口全败、条款被 `launch_editor` 翻绿）已收进 `eval/check_criteria_binding.py`。内容条款绑到自己的扇。`manual` 仍不能由模型文本翻牌
- 关卡和全量资产仍要关编辑器跑 `asset.bat`。编辑器开着时 `refresh_editor_readonly.py` 可以给已有 Actor 标上快照，并用 `find_assets` 补 `/Game` 里还没有的资产卡片。`find_actors` 只返回 `refPath`，不带回坐标，所以不会把快照数字改成当时的实时位置
- 把 `--query` 设成 Cursor 里每句话的默认动作
- 编辑器自己的 Transaction / Undo。写属性失败时写回的是 `props.json` 里那几个字段，不是编辑器事务。索引里仍然没有 Undo 工具，结论仍见 `eval/reports/20261016-171408-cancel.md`
- 两个仓库已有 git。关卡和 `Content/` 不入库。可计入成功率的任务在 `RAG/ue-rag/eval/bench_manifest.json`（改关卡且不保存的不进分子分母）。手工 Cursor 对照列还是空的，见 `Plan/20261015-2100-周计划缺口与面试能力升级.md` 第 1 段
- 打包和部署，见 [`Plan/20261013-1545-打包与部署.md`](Plan/20261013-1545-打包与部署.md)
- 索引刷新不会擅自关编辑器。新建蓝图后写 `index_stale`，编辑器关着才把 `shortcuts\asset.bat` 放进队列；真正拉起要 `UE_INDEX_AUTO_RUN=1`。编辑器开着只提示。L2 的 `linked` 分支会看索引是否过期，过期仍用当前唯一命中，不改猜别的资产
- 菜单 2 已经和菜单 3 共用理解器。离线的 `plan_sequence()` 仍不叫模型。`A` 类索引里没有的蓝图（例如 `ATestActor` 的 `BP_TestActor`）推不出父类时仍暂停，只问一次路径。不按类凭空生成实例

## 换机器

下表是键缺失时的代码缺省。本仓库 `.env.example` 未注释项把后端设成 `monolith`、模型设成 `deepseek-flash`，并把图开关、`UE_ALLOW_AI`、`UE_ALLOW_MONOLITH_WRITE` 写成 true。改 `RAG/ue-rag/.env`，或依赖 `config.py` 的缺省：

| 变量 | 缺省 |
| --- | --- |
| `UE_MCP_URL` | `http://127.0.0.1:8000/mcp` |
| `UE_EDITOR_CMD` | `D:\UEEngline\UE_5.82\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe` |
| `UE_PROJECT` | `E:\UEProject\AIWork\AIAgentProject\AIAgentProject.uproject` |
| `UE_SOURCE_ROOT` | `E:\UEProject\AIWork\AIAgentProject\Source\AIAgentProject` |
| `UE_EXEC_MODE` | `real` |
| `UE_MCP_TIMEOUT` | `300`（裁判） |
| `UE_MCP_CALL_TIMEOUT` | `60`（普通 MCP） |
| `UE_ALLOW_BLUEPRINT` | `false`。`true` 才允许改蓝图 Graph。两条后端都生效 |
| `UE_ALLOW_MATERIAL` | `false`。`true` 才允许改材质图。两条后端都生效 |
| `UE_ALLOW_UMG` | `false`。`true` 才允许改 UMG。两条后端都生效 |
| `UE_ALLOW_ANIM` | `false`。`true` 才允许改动画蓝图和 Control Rig。两条后端都生效 |
| `UE_ALLOW_NIAGARA` | `false`。`true` 才允许改 Niagara。两条后端都生效 |
| `UE_ALLOW_AI` | `false`。只在 `monolith` 时放行 `ai` 的非只读 action |
| `UE_ALLOW_MONOLITH_WRITE` | `false`。只在 `monolith` 时放行其余命名空间的非只读 action |
| `UE_ALLOW_PYTHON` | `false`。`true` 才允许 `editor.run_python`（编辑器内 Python，万能但有越权面）。任务上显式写 `allow_python: false` 的仍然拒绝，这个开关只给任务上没写这个键的路径当缺省 |
| `UE_EDITOR_BACKEND` | `official`。`monolith` 时编辑器走 9316，不回退 8000 |
| `UE_MONOLITH_URL` | `http://127.0.0.1:9316/mcp` |
| `UE_MONOLITH_ROOT` | `E:\UEProject\AIWork\RAG\monolith-master` |
| `UE_DEFAULT_SAVE` | `true`。任务没写 `save` 时是否保存关卡 |
| `UE_DEFAULT_PIE` | `true`。任务没写 `pie` 时是否允许进 PIE |
| `UE_SOLVER_ENABLED` | `false`。没写就是关。本仓库 `.env.example` 写成未注释的 `true`，那是这台机器的值，不是代码缺省 |
| `UE_LLM_MODEL` | `deepseek-chat`。空着也用这个名字 |
| `UE_DISTILL_TEACHER` | 空。设成 `1` 才在非 dry-run 的蒸馏里叫现有模型，而且只写 `candidates.jsonl` |
| `DEEPSEEK_API_KEY` | 空。必须自己填 |

克隆后要跑 `.codex/link_agents_skills.ps1`，否则 Codex 看不到 `.agents/skills`。联接不进 git。`AGENTS.md` 和 `ue-rag-route-skill` 里的 Python 路径仍写着本机 `.venv`。引擎或插件升级后重新跑 `mcp.bat`，否则 `ue_tools` 和编辑器对不上。

不要提交 `.venv`、`data/chroma`、`data/traces`、`.env`。`.env.example` 只列变量名。
