# 工作流使用说明

> **开工协议**：开工先在 `RAG/ue-rag`（用该目录 `.venv`）跑 `python agent/resume_context.py`，它打印 [`progress.json`](../RAG/ue-rag/data/harness/progress.json) 的 goal / phase / next_action、[`feature_list.json`](../RAG/ue-rag/data/harness/feature_list.json) 里第一条 `pending`（`passed` 只由裁判或离线检查改写，不认模型宣布）和最新一份交接的 `unmet`，不超过 200 字；进程概览见 [`documents/harness/overview.md`](documents/harness/overview.md)。本文按标题只读需要的那一节。和后文沿革冲突时，以「当前系统」和 `feature_list.json` 为准。

## 当前系统（2026-10-27）

### 一次运行只连一个编辑器

`backends/select.py` 在入口读一次 `UE_EDITOR_BACKEND`，写进该 run 的 `editor_backend` 事件。本进程之后不跟 `.env` 改口。选错入口直接拒绝，原文带当前后端名，不改连另一边。

| 值 | 地址 | 调用 |
| --- | --- | --- |
| `official`（键缺失时的缺省） | `UE_MCP_URL`，缺省 `http://127.0.0.1:8000/mcp` | `scan_mcp_tools.py` → `call_mcp_tool.py` → `core/editor_exec.py` |
| `monolith` | `UE_MONOLITH_URL`，缺省 `http://127.0.0.1:9316/mcp` | `scripts/monolith_backend.py`，或 `backends/monolith.py` 的 discover / describe / exec |

两边同时在听时，`core/editor_launch.py` 拒绝启动：`monolith` 要关掉 Unreal MCP（ModelContextProtocol），`official` 要关掉 Monolith，然后重启编辑器。PCG 在 Monolith 上没有命名空间，调用失败，不回退 8000。

裁判种类仍是 `editor_assert`、`blueprint_compile`、`cpp_build`、`automation`。`backend=monolith` 时位置走 `mesh.get_actor_info`，蓝图编译走 `blueprint.compile_blueprint`，控件编译走 `ui.compile_widget`，Automation 走 `editor.run_automation_tests`。`official` 仍用原来的工具。

DSH 的 9 个 `mcp__uerag__*` 不增加。`scan_editor_tools` / `get_editor_tool_schema` / `exec_editor_tool` 在 `server/dsh_bridge.py` 转到已锁定的后端。

本仓库 `.env.example` 里未注释的项是当前这台机器正在用的值：`UE_EDITOR_BACKEND=monolith`，模型 `deepseek-flash`，五个图开关、`UE_ALLOW_AI`、`UE_ALLOW_MONOLITH_WRITE` 都是 true，`UE_DEFAULT_SAVE` 和 `UE_DEFAULT_PIE` 都是 true。`.env` 不提交。键缺失时的代码缺省仍是 `official`、`deepseek-chat`、图开关 false、save/pie true。

### 蓝图三次止损

只在 `backend=monolith`，且动作是 `compile_blueprint`、`bind_blueprint`、`create_widget_blueprint`、`edit_widget_blueprint` 时，`agent/sequence.py` 的 `repair_blueprint` 对同一张蓝图、同一个目标最多修 3 次。一次尝试是：还不知道 action 才 discover，然后 describe、exec，再用 exec 结果复查。对应 `UE_ALLOW_*` 不是 true 时次数是 0，步骤写入 `Plan/`。第 3 次仍失败，错误含「已尝试 3 次，Monolith 不具备该能力」，不改连 8000，不换另一张蓝图。`compile_project` 和 `edit_cpp` 不占这 3 次。路径解析失败不计入尝试。`backend=official` 不走这 3 次。

### 诊断句不扩动作表

`agent/diagnose.py` 不叫模型、不写盘、不连编辑器。句子含「看看 / 检查 / 核对 / 是否有问题 / 有没有问题」，且不含「实现 / 新建 / 创建 / 修改 / 摆 / 放置」时，用原句或台账里的 ASCII token 去对源码里已声明的 Automation 过滤器。恰好一条，就收成现有的 `run_automation_test`。零条：该范围仍是缺口。多条：一扇暂停窗，只问一次过滤器全名，不预选。不收成 `implement_shooter` 或 `check_platform_hud`，也不往 `catalog.py` 加行。

### 做到哪

`feature_list.json` 里仍是 pending 的三条：

- `patrol-presentation`：行为树、动画、受击三个 `BlueprintImplementableEvent` 仍由人绑。打开 `UE_ALLOW_AI` 不把这条改成 passed。步骤仍在 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`。
- `editor-transaction`：没有编辑器 Transaction / Undo，属性回滚靠 `props.json`。
- `level-assembly-persist`：整句平台跳跃还不能标成功。金币、平台、门、进度中枢已在 `L_NewTest`，HUD 由 C++ `UPlatformHUDWidget` 驱动。`umg-hud-exec`、`level-assembly`、`platform-automation`、`material-domain`、`umg-domain` 在清单里已是 done。

### 入口

`start.bat` 四条：1 `chat.py`，2 `run_multi.py`，3 `run_windows.py`，4 `--resume latest`。菜单 2 和 3 共用 `plan_windows`。一张 LangGraph。DSH 仍是外壳，不跑这张图。写策略仍只读人写的 `data/dsh/session_task.json`。

动作表在 `agent/catalog.py`。规划者能选的包括：`launch_editor`、`compile_project`、`compile_blueprint`、`edit_cpp`、`bind_blueprint`、`create_level`、`open_level`、`place_actor`、`place_batch`、`remove_batch`、`judge_pickup`、`set_pickable`、`move_actor`、`query_actor`、`set_scalar_param`、`query_material`、`create_widget_blueprint`、`edit_widget_blueprint`、`run_automation_test`、`implement_shooter`、`check_platform_hud`。`integrate_verify` 与 `dispose_residual` 只由代码追加。

保存和 PIE：任务没写时跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`，两边缺省都是 true。`call_mcp_tool.py` 省略 `--save` / `--pie` 同样跟随缺省；`--no-save` / `--no-pie` 才拒绝。

技能正文 24 份，在 `.codex/skills`，含 `ue-monolith-backend-skill` 和四份 DSH 技能。

## 沿革

给拉取本仓库的人。下面按日期记下已经接上的用法。和「当前系统」冲突时，以那一节为准。2026-10-12 接上三条对话、裁判和检索。2026-10-13 把模型名收进配置，技能卡片补到 19 条，评测表开始合计 token，并放入巡逻敌人的 C++ 半边。同一天补上蓝图只读索引、角色上下文切片、一条写图 hook、失败分类文本表，以及成功后的历史和任务记录。2026-10-14 把「编译工程」「启动编辑器」和「能否拾取」拆成独立步骤：编译工程不要求唯一类，拾取只读裁决。2026-10-15 把「句子 → 动作」从正则升级成四层：L0 快路径（手写规则 + 已学句式）→ L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径、自动插创建前置扇）→ L3 学习回写，见 [`Plan/20261015-1705-自然语言理解前置与归地重构.md`](Plan/20261015-1705-自然语言理解前置与归地重构.md)。2026-10-16 把每次模型调用的模型名、耗时、token、错误和响应里真正有的缓存字段记下来，并用同一三条任务对照了两个模型名。观测页由 `report.py` 从已有 trace 生成，四个 HTTP 路由在 `RAG/ue-rag/server/gateway.py`，不跑第二张图。同一天把 `cpp_build` 收成可取消的一次编译：开始时写下进程、开始时间和 300 秒截止，超时或停止都结束进程并记失败。任务书确认和 `s/c/r` 的选择写入事件 `confirm`。2026-10-17 把 L3 从只记句式扩成技能记忆：`data/skills/learned_skills.json` 是主库，`learned_routes.json` 仍可读；失败模式、澄清问法、蒸馏候选和索引队列都在闸门里面。2026-10-19 在同一张 LangGraph 上补上 Harness 缺的四块：决策字段、扇间接交接文件、开工前默认失败的验收合同、回放通过才允许提升的技能。同一批还把契约里的 `query_actor` 和 `run_automation_test` 登记成只读动作，写属性前留下字段快照。供应商、`base_url`、默认不保存、默认不 PIE 都没有改。计划在 [`Plan/20261019-1130-Harness对照与能力缺口升级.md`](Plan/20261019-1130-Harness对照与能力缺口升级.md)。2026-10-20 按 [`Plan/终极蒸馏方案/升级执行/`](Plan/终极蒸馏方案/升级执行/01-信息层.md) 落了三轮升级：信息层（`documents/harness/` 五份文档、`data/harness/feature_list.json` 与 `progress.json`、`ue-harness-session` 会话规则），约束层（`eval/check_architecture.py` 依赖方向测试、`core/risk.py` 三级风险矩阵、`core/fault.py` 熔断、`eval/check_schema.py` 契约校验），自动化层（`agent/resume_context.py` 开工读档、`harness_role` 角色白名单、`agent/garden.py` 园丁、`data/harness/scoreboard.json` 记分板）。机器状态以 `feature_list.json` 为准。

学习路线、岗位对标和 13 周计划在 [`周计划.md`](周计划.md)。那份文档是目标，不是当前操作手册。项目说明在 [`README.md`](README.md)。10 月 13 日第 8 节之后的缺口在 [`Plan/20261013-1310-第8节之后的缺口.md`](Plan/20261013-1310-第8节之后的缺口.md)。打包和部署只写在 [`Plan/20261013-1545-打包与部署.md`](Plan/20261013-1545-打包与部署.md)，还没执行。

## 现在有三条对话

Cursor 里按 `AGENTS.md` 说话的那条还在。人确认 `.cursor/memory/plans/` 之后，可以用 `run_task.py --query` 把改、编译、测试交给闭环。未确认不调用。这不是菜单 3，菜单 3 也不会变成 Cursor 里每句话的默认动作。

不想开 Cursor 时，双击工程根目录的 `start.bat`。四条选项共用同一份检索库和同一个编辑器执行器。`run_agent.py` 还在磁盘上，菜单不再调用它。

| | 普通对话 | 单循环 | 多窗口 |
| --- | --- | --- | --- |
| 入口 | `start.bat` 选 1，或 `python agent/chat.py` | `start.bat` 选 2，或 `python agent/run_multi.py` | `start.bat` 选 3，或 `python agent/run_windows.py`。默认带终端面板，`--plain` 纯文本；菜单 4 直接续接最近一个没跑完的 run，REPL 里 `:resume latest`（或 `:resume <run_id>`）同效 |
| 谁规划 | 检索一次后直接回答 | 菜单 2 的 `run_multi` 和菜单 3 共用 `plan_windows`。手写规则每一步都能落地时仍是 L0，直接跑；弱命中或未命中走 L1 理解器，理解器失败再退回手写步骤。离线检查仍调用 `plan_sequence()`，那条不叫模型 | 分层规划（2026-10-15 起）：L0 快路径离线零模型——手写规则强命中（每一步都能落地才强；拆出任何暂停扇算弱命中），或已提升的 `data/skills/learned_skills.json`，再不行才读 `agent/learned_routes.json` 同构句式，直接走 L2 归地；句里有「实现/逻辑/随机/功能/需求/行为」这类需求说法而没有任何改码扇接住时也算弱命中（181645 的教训：需求半句不许静默丢掉、测试不许替没实现的需求报成功）。L0 不中先过 L1 理解器（`agent/understand.py`，一次 `UE_LLM_MODEL` 调用，只从动作目录选动作、填槽位原始 token、推不出的写进 unresolved），产出经 `agent/intent.py` 验收后由 `agent/grounding.py` 归地：asset 槽位索引唯一命中直连，miss 且父类可推自动插「创建蓝图」前置扇，推不出只问一次 `r`，问法来自 `clarify_policy.json`。L1 说不出合法意图才回退老规划者（重试上限 1 次）。表外动作记成候选，不执行。大任务先出任务书（超过 3 扇 / 新功能重构 / 理解器有 unresolved），人审通过才执行，范围窗口继承 L2 归地候选。任务书「尚未完成清单」读台账、未提升技能和 unresolved |
| 改工程 | 不改 | 不再每步输入 `y`。做不了或失败时暂停，可以补一次路径 | 每扇新的 `thread_id`。消息是这一扇的任务句；有上一扇交接文件时，前面再附一份短 JSON（`unmet`、证据种类、`next`），不附工具正文。写编辑器或 `cpp_build` 的窗口过编辑器闸门串行，只读裁决和只读契约动作并行 |
| 验证 | 不编译 | 和以前的多 Agent 相同。模型不能自己宣布成功 | 集成不叫模型。某一扇失败只重试这一扇。任务书确认后代码追加 `integrate_verify` 跑矩阵验收 |
| 记录 | 不写 trace。人在 `s/c/r` 里选择时，往 `data/events/<同一次 run id>.jsonl` 追加 `confirm` | 每步写在 `data/runs/YYYYMMDD/`（按日期分目录，老 run 原平铺兼容读写）。图内部 trace 仍在 `data/traces/*.jsonl` | 同一目录，按窗口分节，带上依赖和 `{ok, summary}`。另有 `data/runs/YYYYMMDD/<run_id>.json` 机读状态、`data/events/<run_id>.jsonl` 事件流、`data/archive/<run_id>/` 归档 |

目标仍是 [`周计划.md`](周计划.md) 第 2.1 节那句验收：一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人。检索、改 C++、编译、读错误、再改、再测由闭环做。行为树、动画和受击表现由人在编辑器里绑，对应合同里的 `manual` 条款，模型文本改不了 `passed`。这句话的 Automation 条款现在会生成，过滤器是 `AIAgent.Enemy.Patrol`；菜单开跑时代码再追加一扇只读 `run_automation_test`，不因此把 `pie` 设成 true。过滤器失败时该条款保持 `passed: false`。自动条款都过了、人工条款还在时，整句记「部分完成」。C++ 半边是 `UTestEnemyPatrolComponent` 和只负责挂载的 `ATestEnemy`：绕出生点巡逻，发现玩家后追击，近距离调用 `OnMeleeAttack`。三个事件都是 `BlueprintImplementableEvent`，图由人接。窄闭环已经能对三条固定任务做编译和断言。巡逻这条的读数画面在 `RAG/ue-rag/eval/reports/demo-c-patrol-20261016-124807.mp4`，文字来自 trace `demo-c-patrol-20261016-124807.jsonl`。

`chat.py` 的 `answer`、`nodes.py` 的 `plan_node`、`supervise.py` 的 `_from_model` 和理解器都经 `RAG/ue-rag/core/llm_call.py` 调用模型。模型名每次读 `UE_LLM_MODEL`。键缺失时是 `deepseek-chat`。本仓库 `.env.example` 写的是 `deepseek-flash`。供应商和 `base_url` 没有改。每次调用把模型名、耗时、prompt token、completion token、错误写进 `data/costs.jsonl`。响应里如果有 `prompt_cache_hit_tokens`、`prompt_cache_miss_tokens` 或 `cached_tokens`，一并写下；没有就空着，不估算。trace 里同一次 `plan` 也会带上这些字段。

## 一句多步

菜单 2 是单循环，菜单 3 才是多窗口。两边都还是原来那张 LangGraph，没有第二张业务图。图的边没有加。模型不能自己宣布成功。路径对不上仍然不猜。

菜单 2 现在和菜单 3 共用 `plan_windows`、动作表和编辑器执行器。手写规则仍然是 L0 种子：每一步都能落地就直接跑。弱命中或未命中才走理解器；理解器说不出合法意图时，先退回这些手写步骤，再不行才是老规划者。`plan_sequence()` 本身仍只走手写规则和已学句式，离线检查用它，不叫模型。每一步还是同一张图。

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

动作表以「当前系统」那一节为准，定义在 `agent/catalog.py`。schema 用 `contract/tools.json` 里已有的那一份，不读 `ue_mcp_index.json`。`query_actor` 和 `run_automation_test` 不在 `allow_writes` 里也能读，写前缀规则没有放宽。其余四个契约工具（`query_blueprint`、`find_asset`、`build_project`、`get_build_error`）仍只给网关，不进动作表。官方 830 个工具只在 `backend=official` 的扫描路径；`backend=monolith` 走 discover / describe / exec。表外动作暂停，记成候选，不执行，也不写进快路径。诊断句归地失败也不为此加行。人审并补上验证之后才算入表。可实现指槽位已经唯一，不是指场景里一定有实例，也不是指编译已经通过。场景里没有实例、编译失败、裁决是「不能」，都不删已经记下的句式。人要作废就删 `learned_skills.json` 里的那一条；旧的 `learned_routes.json` 仍可单独删。

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

REPL 里 `:quit` 退出，`E` 展开上一个 run 的每扇明细（状态、ok、elapsed、retries），`:resume latest`（或 `:resume <run_id>`）续接——误敲成 `--resume ...` 或裸写 `resume latest` 也按续接处理，别的 `--xxx` 只提示不执行，不再当任务句进规划者（2026-10-14 185550、190429 两次白烧规划者调用后补的）。`start.bat` 菜单 4 直接续接最近的 run。续接时面板读得到上一扇的 `unmet`；交接文件被删掉也不影响 checkpoint。`eval/check_routes.py` 现在同时跑 `eval/check_upgrade.py`：离线覆盖六段（事件流与机读状态、面板渲染、任务书与人审门、续接与快照还原、环/悬空诊断、只读并行、工件、矩阵台账归档）加 build 前打转熔断与 REPL 指令，通过时打印 `upgrade ok` 和 `routes ok`，不叫真实模型、不写场景。蒸馏契约在 `eval/check_distill.py`，通过时打印 `distill ok`。Harness 字段、合同、交接、上下文、回放、只读动作和属性快照在 `eval/check_harness.py`，通过时打印 `harness ok`。`python agent/distill.py --dry-run` 只统计候选，不叫模型。`UE_DISTILL_TEACHER=1` 且不是 dry-run 时，才用现有 `UE_LLM_MODEL` 把候选收成技能 JSON，追加进 `data/distill/candidates.jsonl`，不写 `learned_skills.json`。巡逻敌人的蓝图步骤写在 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`，不自动绑图。

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
start.bat ─┬─ 普通对话 chat.py ─ retrieve ─ 回答
           ├─ 单循环 run_multi.py ─ plan_windows（与菜单 3 同一条）─ 每步同一张图
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
| 执行 | `backends/select.py`、`core/editor_exec.py`、`backends/monolith.py` | 一次运行锁一个后端。`official` 走 `editor_exec.py` 到 8000。`monolith` 走 discover / describe / exec 到 9316。选错入口拒绝 |
| 本地补丁 | `core/source_edit.py` | `read_source`、`apply_unified_diff`。路径锁在 `Source/AIAgentProject` |
| 裁判 | `core/judge.py` | `{ok, kind, summary, errors:[{file, line, message}]}` |
| 图 | `agent/graph.py`、`agent/nodes.py`、`agent/roles.py` | 节点和边不变。`plan` 按任务种类换提示和工具。trace 记 `role` |
| 监督者 | `agent/supervise.py`、`agent/plan_windows.py`、`agent/catalog.py`、`agent/intent.py`、`agent/understand.py`、`agent/grounding.py`、`agent/diagnose.py`、`agent/sequence.py`、`agent/skill_memory.py`、`agent/failure_memory.py`、`agent/clarify_policy.py`、`agent/distill.py`、`agent/harness.py`、`agent/roles.py`、`agent/garden.py`、`agent/resume_context.py` | L0 手写规则（每步都落地才算强命中）先拆，然后已提升技能，再然后 learned_routes。弱命中/未命中过 L1 理解器（动作卡片 + 原句 + 3 条检索标题，1KB 上下），意图经 `validate_intent` 验收后 L2 归地：asset 槽位 `link_asset` 索引命中直连并看索引是否过期、类可推自动插创建前置扇、推不出问一次 `r`。只读诊断句在补窗前由 `diagnose.py` 收成唯一过滤器的 `run_automation_test`。L1 失败才叫老规划者（看不到工具原文），重试上限 1 次。整句集成成功、unresolved 为空、并且回放没有变差，才提升技能（提升必须带 `source_run`，空值停在 shadow；提升写 `source_runs` 数组和 `content_sha1`）。`fix` 先查 `failure_patterns.json`，症状对不上再用 `ue_error`。蒸馏读 run、event、trace 和 `data/archive` 的哈希与矩阵结论，失败样本保留 `error Cxxxx`。默认不叫教师模型。任务可带 `harness_role`：`research` / `review` 只读，写工具一律拒（`allow_writes` 里写了也拒）；`plan` 只返回窗口清单；没写时行为与原来完全一致，仍是同一张图。`garden.py` 只扫文档抬头、记 stale，`resume_context.py` 开工读档。`sequence.repair_blueprint` 在 monolith 下对蓝图类动作最多 3 次 |
| 网关 | `server/gateway.py` | 127.0.0.1:8010 上四个路由：`/api/health` 返回模型名和 MCP 是否可连；`/api/chat` 一次补全；`/api/structured` 包理解器；`/api/tool-call` 只转发契约里的六个工具。不拆窗口，不跑 LangGraph |
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

源码用 `data/snapshots/<run_id>/<window_id>/` 的字节快照还原。`set_pickable` 和 `move_actor` 另把写入前的 `bPickable`、`Tags` 或位置记在 `data/snapshots/<run_id>/props.json`。这一扇失败且任务没有 `keep_source` 时写回这些字段，写回后按任务策略保存关卡（缺省保存）。同一 `ref` 记了多次就不写回。索引里仍然没有 Transaction / Undo 工具，这条快照不代替编辑器事务。

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
│   ├── start.bat            四条入口：对话、单循环、多窗口、续接
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
    ├── server\              检索 MCP、六工具契约、DSH 网关、四个 HTTP 路由（8010，不跑图）
    └── data\                JSONL、技能卡片、Chroma、traces、costs.jsonl、harness\（feature_list、progress、risk_matrix、arch_whitelist、scoreboard、garden_report、circuit）
```

| 目录 | 谁读 | 干什么 |
| --- | --- | --- |
| `AGENTS.md` | Cursor 与 Codex | 铁律和路由。`.codex/AGENTS.md` 只是指针，仓库根开工时 Codex 不加载它 |
| `.cursor/rules/*.mdc` | Cursor，始终生效 | 动编辑器先看 `UE_EDITOR_BACKEND`：`official` 先扫描工具，`monolith` 走 discover / describe / exec。改 UE 项目先计划、后记录 |
| `.codex/instructions/` | Codex，`SessionStart` 注入 | 与上面四份规则对等的短文 |
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

这些挡在「任意一句话生成一个会巡逻的敌人」前面，不挡上面三条窄任务，也不挡已经编译过的 `ATestEnemy`：

- `patrol-presentation` 仍是第一条 pending。C++ 有巡逻、追击和近战事件，合同里有一条默认失败的 `AIAgent.Enemy.Patrol`。三个 `BlueprintImplementableEvent` 不自动连线。`.env.example` 里 `UE_ALLOW_AI` 已经是 true，这条仍不因此改成 passed。自动条款过了、人工条款还在，整句是「部分完成」。步骤仍在 `Plan/20261017-1507-巡逻敌人蓝图人工步骤.md`
- `backend=monolith` 且对应写开关为 true 时，蓝图类动作最多试 3 次，不保证成功。第 3 次失败就停，不改连 8000。开关不是 true 时次数是 0，步骤写入 `Plan/`。本轮没有改三个近战事件的图
- 平台跳跃的 `material-domain`、`umg-domain`、`platform-automation`、`level-assembly`、`umg-hud-exec` 在 `feature_list.json` 里已是 done。整句仍不能标成功：`level-assembly-persist` 还是 pending。`check_platform_hud` 只核对 `WBP_HUD` 里有没有 `txt_CoinCount`、`txt_Timer`、`txt_Clear`、`txt_Fail`
- 交叉编码器重排、换 Embedding、Docker。观测页是 `report.py` 从已有 trace 写出的静态 HTML，不是常驻网站
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
| `UE_EDITOR_BACKEND` | `official`。`monolith` 时编辑器走 9316，不回退 8000 |
| `UE_MONOLITH_URL` | `http://127.0.0.1:9316/mcp` |
| `UE_MONOLITH_ROOT` | `E:\UEProject\AIWork\RAG\monolith-master` |
| `UE_DEFAULT_SAVE` | `true`。任务没写 `save` 时是否保存关卡 |
| `UE_DEFAULT_PIE` | `true`。任务没写 `pie` 时是否允许进 PIE |
| `UE_LLM_MODEL` | `deepseek-chat`。空着也用这个名字 |
| `UE_DISTILL_TEACHER` | 空。设成 `1` 才在非 dry-run 的蒸馏里叫现有模型，而且只写 `candidates.jsonl` |
| `DEEPSEEK_API_KEY` | 空。必须自己填 |

克隆后要跑 `.codex/link_agents_skills.ps1`，否则 Codex 看不到 `.agents/skills`。联接不进 git。`AGENTS.md` 和 `ue-rag-route-skill` 里的 Python 路径仍写着本机 `.venv`。引擎或插件升级后重新跑 `mcp.bat`，否则 `ue_tools` 和编辑器对不上。

不要提交 `.venv`、`data/chroma`、`data/traces`、`.env`。`.env.example` 只列变量名。
