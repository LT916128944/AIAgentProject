# AIAgentProject

UE 5.8 工程，给 AI Agent 一个可操作的编辑器和一份 C++ 游戏模块。检索、对话和执行闭环在旁边的 [ue-rag](https://github.com/LT916128944/ue-rag)。

| 文档 | 管什么 |
| --- | --- |
| 本文 | 仓库里有什么、怎么和 ue-rag 放在一起 |
| [`WORKFLOW.md`](WORKFLOW.md) | 已经接上的对话、检索、裁判和评测。开头「当前系统」与后文沿革冲突时，以那一节为准 |
| [`周计划.md`](周计划.md) | 学习路线、岗位对标、13 周计划 |
| [`AGENTS.md`](AGENTS.md) | Cursor 与 Codex 共用的工具路由和 Token 节流 |
| [`../RAG/ue-rag/dsh/README.md`](../RAG/ue-rag/dsh/README.md) | DeepSeek Harness 外壳：安装、9 个工具、写策略、排障 |
| [`documents/harness/overview.md`](documents/harness/overview.md) | 进程概览。做到哪一步看 `feature_list.json` |

## 目录怎么放

入口页 `server/static/entry.html` 在本仓库**之外**（在 `RAG/ue-rag` 仓库里），由同仓库的 `server/entry.py` 提供。两个仓库克隆之后要保持同级：

```text
AIWork\
├── AIAgentProject\          本仓库
└── RAG\ue-rag\              https://github.com/LT916128944/ue-rag
```

## 这个仓库里有什么

- `AIAgentProject.uproject`：引擎关联 5.8。官方 Unreal MCP 与 Monolith 插件一次只开一个，跟 `RAG/ue-rag/.env` 的 `UE_EDITOR_BACKEND` 一致。
- `Source/AIAgentProject/`：游戏模块。巡逻敌人的 C++ 半边是 `ATestEnemy` 和 `UTestEnemyPatrolComponent`，发现玩家、追击、近战各有 `BlueprintAssignable` 多播委托（`OnPatrolDyn` / `OnChaseDyn` / `OnMeleeAttackDyn`），`BP_TestEnemy` 的 EventGraph 已接上（剩余是跑出带 `evidence_sha1` 的 run 记录）。旁边还有测试用的 Pawn、Character、GameMode、PlayerController。
- `Content/`：测试关卡和蓝图，含 `BP_Gun`、`BP_Sphere`、`BP_Character`、`BP_GameMode`、`Maps/L_NewTest`。
- `.cursor/`、`.codex/`：域技能和规则。技能正文以 `.codex/skills` 为源，当前 24 份，同步到 `.cursor/skills`。Codex 的始终生效短文在 `.codex/instructions/`，钩子在 `.codex/hooks.json`。发现技能前跑 `.codex/link_agents_skills.ps1`。
- `documents/`：功能设计。导出后进入 ue-rag 的检索。
- `Plan/`：过程记录，不进入检索。
- `start.bat`：本仓库唯一的入口——双击起入口页（起服务 + 服务自己开浏览器）。只做启动，不做菜单；三个模式在页面上选。页面本身在 `RAG/ue-rag/server/static/entry.html`。

## 首次克隆后先构建（init）

> 本仓库只提交源码和资源，**不含任何编译产物**：`Binaries/`、`Intermediate/` 已被 `.gitignore` 忽略——它们体积大、且与本机引擎/工具链强绑定，本就不该进 git。

1. **环境**：UE **5.8**（`AIAgentProject.uproject` 的 `EngineAssociation` = 5.8）+ **Visual Studio 2022** 的「使用 C++ 的游戏开发」工作负载。
2. **首次打开会重编插件**：用 UE5.8 打开 `AIAgentProject.uproject`，Monolith 没有对应引擎版本的二进制，编辑器会提示重建模块，点 **Yes** 即从源码编译（20 个模块，首次较慢）；之后正常打开不再重编，除非改了源码或换了引擎版本。
3. **可选第三方插件缺了也能编**：`BlueprintAssist` / `BlueprintAttributes` / `ComboGraph` / `SMSystem` 等缺失不会报错——相关模块在 `Build.cs` 里做了探测，缺了就按 `WITH_* = 0` 降级（例：`WITH_COMBOGRAPH=0`），只是功能相应变少。
4. **`Tools/` 下的 `monolith_proxy.exe` / `monolith_query.exe` 也被忽略**：只有走 MCP 的 proxy / 离线查询时才需要自己跑 `build.bat` 生成；编辑器内的 MCP server 是编进模块的，不影响启动。
5. 上面只保证「拿到工程能编过」。Python 环境、`.env`、检索库和入库这些准备见下一节「准备」。

## 准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`。一次运行只连一个编辑器 MCP。本仓库 `.env.example` 正在用的是 `UE_EDITOR_BACKEND=monolith`，地址 `http://127.0.0.1:9316/mcp`，这时关掉官方 Unreal MCP 插件。改回 `official` 时反过来：开 `http://127.0.0.1:8000/mcp`，关掉 Monolith。两边同时在听，启动会被拒绝。
2. 按 [ue-rag 的 README](https://github.com/LT916128944/ue-rag) 准备 Python 3.12 虚拟环境、依赖和 `.env`。换机可以直接跑 `RAG/ue-rag/shortcuts/setup.ps1`：找到 Python 3.12，建 `.venv`，装依赖；`.env` 没有才从 `.env.example` 复制。至少填 `DEEPSEEK_API_KEY`。密钥不要提交。复制 `.env.example` 得到的是这台机器正在用的值：模型 `deepseek-flash`，蓝图 / 材质 / UMG / 动画 / Niagara / AI / Monolith 写开关都开，保存和 PIE 开，求解开关 `UE_SOLVER_ENABLED=true`（`.env.example` 里这一行是未注释的 `true`，写的就是这台机器的值，不是代码缺省）。键没写时，代码缺省仍是 `official`、`deepseek-chat`、图开关关闭、求解关闭。保存和 PIE 的代码缺省是开。装完后跑 `python eval/check_all.py`、`shortcuts/ingest.bat`，再按下一节起入口页。不下载 UE。
3. 第一次使用前的入库、工具索引和三种模式的差别，按 [`WORKFLOW.md`](WORKFLOW.md) 的「拉取之后先准备」。

准备好之后一步起入口页（服务会自己打开浏览器）。**双击 `start.bat` 就行**，它替你做 `cd` 与解释器路径：

```text
双击 AIAgentProject\start.bat
```

等价的命令行写法：

```powershell
cd RAG\ue-rag
.\.venv\Scripts\python.exe server\entry.py --port 8011
```

- 页面在 `http://127.0.0.1:8011/` 自动打开，不用手敲。`--no-open` 关掉自动打开（脚本 / 无 GUI）。
- 这条命令再跑一次不会撞端口：检测到已有实例就直接把页面打开，退 0。
- `start.bat` 只做「起服务」一件事，开页由服务在端口真 listen 之后自己做（`entry.py::_open_when_ready`）。启动前它会检查 `.venv` 与 `server\entry.py` 在不在，缺了直接说该跑 `shortcuts\setup.ps1`。
- `start.bat nopause` 跑完不暂停（脚本 / CI）；`start.bat --port 8012`、`start.bat --no-open` 等其余参数原样转给 `entry.py`。
- 直接双击 `RAG/ue-rag/server/static/entry.html` 打不开功能——`file://` 页面无法拉起本地进程，`/api/*` 全在后端起。所以是**服务起页面**，不是页面起服务。

页面输入区下面选模式：

| 模式 | 行为 |
| --- | --- |
| 多窗口 | `run_windows.py`。整句才命中规则；盖不住则求解或停在缺口，然后分扇执行 |
| 单循环 | `run_multi.py`。与多窗口共用拆句，每步走同一张图 |
| 普通对话 | `chat.py`。检索后回答，不改工程，不编译 |

「续接最近」读最近一个没跑完的多窗口 run。三种模式共用 `plan_windows`。写场景之后是否保存、玩法验收是否进 PIE，跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`（缺省都开）。任务里显式写 `save: false` / `pie: false` 才关。

## 工作流怎么用

一句话结论：

- **编辑器一次只走一个后端**。本机 `.env.example` 是 Monolith（9316）。Cursor 在 `monolith` 时读 `ue-monolith-backend-skill`，用 discover / describe / exec；`official` 时才扫描官方工具。选错入口就停。
- **闭环仍是入口页的多窗口 / 单循环**。两种模式共用 `plan_windows`。页面的保存和 PIE 缺省开；DSH 是外壳，写操作只认人写的 `session_task.json`，文件没有就拒绝。9 个 `mcp__uerag__*` 会转到当前锁定的后端，但不跑 LangGraph。改工程和编译回到 `run_windows.py`。DSH 和入口页的编辑器写共用 `core/editor_lock.py` 那一把锁。
- **句子到窗口**仍是 L0 手写规则或已学技能 → L1 理解器 → L2 归地 → L3 回写。门只认整句（2026-11-11 起）：规则 / 已学技能接不住的半句交回模型，不跟整句一起被领走；理解器 goals 盖不住时不再叫规划者重点一遍动作表——求解开着进求解兜底，关着停在缺口。只读诊断句另由 `agent/diagnose.py` 归到唯一的 Automation 过滤器，收成已有的 `run_automation_test`，不往动作表加行。
- **蓝图在 Monolith 下同一目标最多 3 次**。第 3 次仍失败就停，不改连 8000。写开关关掉时次数是 0，步骤写入 `Plan/`。

### 先选入口

| 你要做的事 | 入口 | 一次花几次模型调用 |
| --- | --- | --- |
| 固定句式的窄任务（编译工程、编译一张蓝图、移动 Actor、拾取裁决、放 Actor） | 入口页「多窗口」/「单循环」 | L0 命中 0 次；弱命中 1 次理解器 |
| 新需求，要拆句、要改 C++、要摆场景 | 入口页「多窗口」（`run_windows.py`），先出任务书人审 | 1 次理解器 + 每扇图内 plan 若干次 |
| 表外需求（动作表没有对应动作，如改射速/改常量） | 同「多窗口」：理解器 goals 盖不住整句时直接进求解兜底，不再叫规划者重点一遍动作表。代码缺省 `UE_SOLVER_ENABLED=false`，本机 `.env.example` 为 true。方案预演（只读）里有写 C++ 或改关卡先出任务书人审一次，任务书引用这份方案。求解扇走「盘点 → 源码定位 → 步骤规格 → 编译回灌 → automation 收口」，方案没给过滤器就再补选一次真测试名。台账 `done` 要有改动证据。入口层的 `UE_RAG_STDIN` 在子进程打开前改名为 `.consumed`，第二个人读不到；有活心跳时退出码 2，带 `--force` 才继续并写下 `forced_by` | 1 次理解器 + solve 扇 ≤6 次模型 |
| 只读研究、方案对比、报错定位（不改工程） | DeepSeek Harness（DSH）里的 `search_project` / `workflow` | 检索 0 次；研究模板 5 个子代理 |
| 单步编辑器操作 | 看 `UE_EDITOR_BACKEND`。`monolith` 用 `scripts/monolith_backend.py`；`official` 用扫描脚本。DSH 里仍是原来的 3 个编辑器工具，内部转到当前后端 | 每步 ≥1 次 |
| 续接没跑完的多窗口 run | 入口页「续接最近」 | 只跑没完成的扇 |
| 「看看有没有问题」这类只读诊断 | 「多窗口」。归地成功就是一扇 `run_automation_test` | 诊断归地本身 0 次 |
| 想在浏览器里拼提示词、看已有 run 的卡片与交接 | `python RAG/ue-rag/server/entry.py --port 8011`，开 `http://127.0.0.1:8011`。侧边栏 + 拖拽附件 + 装配预览 + 运行面板 + 决策面板 | 0 次（装配是纯函数） |
| 想从浏览器起一次 run、答人审门 | 同上一行，点「发送」：写 `data/ui/req/<ts>.txt` → 子进程 `python agent/run_windows.py --plain --force`；裁决写 `data/ui/inbox/<run_id>.jsonl`（一行一条，双键领取）。**不是第二条执行链**：仍是那个 CLI 进程在跑 | 与「多窗口」相同 |

### 一条 UE 需求的标准步骤

以「敌人发现玩家后播放追击动画」这类需求为例：

0. **预检**（DSH 外的终端，或双击 `..\RAG\ue-rag\shortcuts\dsh.bat`）：venv、`.env`、配置、技能、索引、当前后端的端口有一项不过就先修那一项。Monolith 时确认 9316 在听、8000 没在听。
1. **授权文件**：`RAG/ue-rag/data/dsh/session_task.json` 不存在时所有写操作被拒（fail-closed）。从 `session_task.example.json` 复制一份，第一次先保持 `allow_writes: []`；确实要写编辑器时再由人补上白名单、`save`、`pie`。
2. **开工读档**：跑一次 `python agent/resume_context.py`，或在 DSH 里调 `resume_context`。拿到 goal / phase / 下一条 pending / 最新交接的 unmet（≤200 字）。不要读 `WORKFLOW.md` 全文、不读 `ue_mcp_index.json`、不拉全量 schema。
3. **只读收敛范围**：`search_project(query, top_k≤5)` 查资产、C++、蓝图图、文档；排障用 `where={"doc_kind":"history"}` 或 `debug`，`top_k=1`。还不够就贴 [`../RAG/ue-rag/dsh/workflows/research.js`](../RAG/ue-rag/dsh/workflows/research.js) 到 DSH 的 `workflow` 工具。
4. **落盘任务书/计划**：需求会改工程时，按 `ue-cpp-plan-skill` 写 `Plan/YYYYMMDD-HHMMSS-<名字>.md` 或 `.cursor/memory/plans/`，等人确认。
5. **执行**：确认后在入口页点「发送」（模式「多窗口」）。写编辑器或编译串行，只读裁决并行。窄任务用「单循环」。
6. **要证据**：`read_contract(run_id)` 看条款，再对未过且非 `manual` 的条款调 `verify(run_id, criterion_id)`。只有返回 `{ok:true, kind}` 的条款才算完成。
7. **人工部分**：仍标成 `manual` 的条款由人在编辑器里处理；`OnPatrol` / `OnChase` / `OnMeleeAttack` 的表现接线已在 `BP_TestEnemy` 的 EventGraph 接上，剩余是跑出一条带 `evidence_sha1` 的 run 记录，步骤清单是 `Plan/patrol-presentation.json`。翻牌只认人写的 `data/attestations/<criterion_id>.json`，模型文本不把 `manual` 改成 passed。
8. **收尾**：多扇任务写交接 JSON，用 `python agent/dsh_cli.py handoff-validate` 和 `handoff-merge`。改了 C++ 或关卡再补当天 `.cursor/memory/任务记录/`。

### DSH 接上了什么

工具面仍是 9 个：`search_project`、`scan_editor_tools`、`get_editor_tool_schema`、`exec_editor_tool`、`read_contract`、`verify`、`read_handoff`、`window_state`、`resume_context`。后三个编辑器工具在 `server/dsh_bridge.py` 转到已锁定的后端，个数不增加。

写闸门仍在：`session_task.json` 缺失时 `exec_editor_tool` 回 `write refused`，并带 `policy`。`allow_writes` / `save` / `pie` 不是工具参数。

不接官方 830 个工具的全量 schema，也不把 Monolith 服务器配进会注入全部 schema 的客户端。DSH 不替代 `run_windows` / `judge`。

### token

结构没变：L0 命中是 0 次模型调用；只读检索是 0 次；新需求要过理解器，再按扇跑图内 `plan`。合计数字以 [`scoreboard.json`](../RAG/ue-rag/data/harness/scoreboard.json) 里现有的那一版为准：26 个任务、24 个 ok、每个成功任务约 126k prompt tokens。这份文件之后没有新的合计写进去。DSH 与菜单 2 的对照在 [`dsh_ab.json`](../RAG/ue-rag/data/harness/dsh_ab.json)，DSH 侧仍是 null，不能据此说 DSH 更省。

固定句式和窄任务可以压在理解器加一轮 `plan`。任意新需求会按扇叠加。表现条款里的 `manual` 仍要人绑；没签时整句是 `partial`，不是 `ok`。

### 红线

- 模型文本不改 `passed`。`manual` 由人在编辑器里绑，步骤在 `Plan/`；翻牌只认人写的 `data/attestations/<criterion_id>.json`。`feature_list.json` 的 `passed` 不由人签函数改写。
- 一次运行只连 `UE_EDITOR_BACKEND` 选定的那个端口。选错入口就停。
- 不用 DSH 的 read/write 直接改 `.uasset` / `.umap` / `Source/`。UE 写只经 `mcp__uerag__*` 网关。
- DSH 会话不写 `allow_writes`、不写 `save` / `pie`、不写 `.env`。
- 诊断句不往动作表加行。归不上就是缺口。
- 入库、导出、索引刷新放在 DSH 外的终端跑。

### 排障

| 现象 | 处理 |
| --- | --- |
| `write refused` + `policy` | 先看 `data/dsh/session_task.json` 是否存在、`allow_writes` 是否含该工具 |
| 错误原文含 `UE_EDITOR_BACKEND=monolith` 或 `=official` | 这次调用走错了入口。改用当前后端的脚本，不改连另一边 |
| 官方 MCP 与 Monolith 同时在听 | 关掉 `.env` 里没选的那个插件，重启编辑器 |
| 预检报 `chroma` readonly，或 shell 写文件报 Access denied | DSH shell 以低完整性级别跑、写不进 `data/chroma`。预检和入库放到 DSH 外的普通终端 |
| 蓝图窗口错误含「已尝试 3 次，Monolith 不具备该能力」 | 同一目标已经用完 3 次。不第 4 次，不改连 8000 |
| 扫描两轮仍对不上工具 | 停。`monolith` 时先 `--discover` 再 `--describe` 一个 action，不回退全量 schema |

校验命令（工作目录 `RAG/ue-rag`，Python 用该目录 `.venv`）：

```powershell
python shortcuts/dsh_preflight.py
python eval/check_dsh_config.py
python eval/check_dsh_bridge.py
python eval/check_backend.py
python eval/check_diagnose.py
```

## 现在做到哪

完成与否看 [`RAG/ue-rag/data/harness/feature_list.json`](../RAG/ue-rag/data/harness/feature_list.json)，不看模型正文。当前仍 pending 的两条：

- `patrol-presentation`：C++ 状态机（`TickPatrol` / `TickChase` / `TickAttack`）和 `AIAgent.Enemy.Patrol` 自动化已经通过，行为树不再建。2026-10-27 那条「monolith 词表缺覆写节点」的归因已作废：真因是 `BlueprintImplementableEvent` 只能由声明它的类自身实现（事件声明在 `UTestEnemyPatrolComponent`，`BP_TestEnemy` 够不到）。2026-11-12 已给组件加三个 `BlueprintAssignable` 多播委托并在调用点 `Broadcast`，`BP_TestEnemy` 的 EventGraph 已接 `AddDelegate` + `CustomEvent`，PIE 实测有 patrol 表现行。**剩余是把这条跑出一条带 `evidence_sha1` 的 run 记录**（`bench_record` 只收 `passed=true` 且带印章的条款）；步骤清单仍留 [`Plan/patrol-presentation.json`](Plan/patrol-presentation.json)。`.env` 里 `UE_ALLOW_AI=true` 不把这条改成 passed，人签文件也不改 `feature_list.json` 里这条的 `passed`。
- `editor-transaction`：没有编辑器自己的 Transaction / Undo。失败时按快照做补偿（方式是 `compensating`），不代替 Undo。这条仍是 pending。

`level-assembly-persist` 已是 done（`data/runs/20261027/20261027-191740.md`）。整句在 monolith 下放置、五条内容条款各绑自己的扇、关卡已保存。已放进关卡的实例要读或改属性时，拆句转入 unresolved，不进执行窗口。

合同假通过的那一种形状（窗口全败、条款被「编辑器已打开」翻绿）由 `eval/check_criteria_binding.py` 钉住。翻牌带判词 `evidence_sha1`，回放时改一个 summary 字节就回到 false。整句收尾是 `ok` / `partial` / `failed`（退出码 0 / 3 / 1）；未签 `manual` 不是 `ok`。退出码 2 仍是有活心跳。动作在两条后端上怎么跑，由 `eval/check_backend_coverage.py` 声明：除 `judge_pickup`、`set_pickable` 因读不到实例属性而标 `partial` 外，清单里的动作是 `both`。monolith 下这两条在拆句转入 unresolved；官方后端仍可进窗口。写前快照是补偿回滚，同一幂等键只做一次，旧值冲突或逆操作失败时写 `manual_intervention` 并停。

检索、改 C++、编译、读错误、再改、再测这条窄闭环可以对固定任务做编译和断言。目标句「一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人」的表现接线已在 `BP_TestEnemy` 的 EventGraph 用多播委托接上，剩余是跑出一条带 `evidence_sha1` 的 run 记录；C++ 行为本身已经在跑。

另一层「全域 Spec 内核」的地基（2026-11-17 边界升级）：九域（UMG / 蓝图图 / 动画蓝图 / 材质 / Niagara / 动画&ControlRig / Sequencer / 后处理 / C++）各有 `agent/<域>_spec.py`，共用 `agent/spec_pipeline.py` 的「Spec→确定性执行→只读复核→回灌修复→沉淀」流水线与 `agent/spec_registry.py` 登记表；工具名用 `agent/spec_toolmap.py` 对当前 monolith 后端做**真机核实**（快照在 `RAG/ue-rag/data/harness/monolith_tool_inventory.json`）。今天算法侧做不到的子形态（材质图连线、ControlRig rig 图、Niagara 系统模块栈、Sequencer 的 Transform/CameraCut 轨道写入）走结构化暂停，不假成功。这层是「兜住能力」的地基，**尚未接进多窗口的动作表**（提升为 CATALOG 动作是后续一步）。细节见 [`../RAG/ue-rag/README.md`](../RAG/ue-rag/README.md) 的「全域 Spec 内核」与 [`Plan/20261117-1630-全域边界升级-落地记录.md`](Plan/20261117-1630-全域边界升级-落地记录.md)。

同日（2026-11-17）还收了几处拆句 / 动作 / 入口的缺口：**连接词层**（`agent/connectives.py`，然后 / 并且… 零模型切句、不自动排依赖）、**缺信息先问模型**（`agent/model_fill.py`，补不出再问人）、**`place_light` 放灯**（新增动作，按类生成原生灯 Actor 并按场景统计 +1 复查）、**HUD 单读数**（只加一条血条也走真控件、配图形血条 `HealthBarBG`/`HealthBarFill`）、**路线图 `launch → open` 串行依赖**、**范围级条款进集成**，以及入口页「输入指令卡死 / 像卡死」修复（发送按钮接线、空答案确认、先落空壳 run、后台日志与预估倒计时）。这些都有离线守卫（`eval/check_{connectives,model_fill,place_light,cpp_edit_scene,roadmap_deps,hud_single_readout,integrate_scope_criteria,entry_send,entry_brief_confirm,entry_run_scaffold,entry_ui_feedback}.py`），细节见 [`WORKFLOW.md`](WORKFLOW.md) 的「2026-11-17 其余收口」与 [`../RAG/ue-rag/README.md`](../RAG/ue-rag/README.md)。

细节以 [`WORKFLOW.md`](WORKFLOW.md) 开头的「当前系统」为准，不要用 [`周计划.md`](周计划.md) 第 4.3 节那张开工时的缺口表判断当前仓库。分数以 `RAG/ue-rag/eval/check_scorecard.py` 打印的为准，写在 `data/harness/scorecard.json`。没有人签、基准档案没跑满时，对应维度停在现状分。
