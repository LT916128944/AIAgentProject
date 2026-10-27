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

`start.bat` 按相对路径找上一级的 `RAG\ue-rag`。两个仓库克隆之后要保持这个位置：

```text
AIWork\
├── AIAgentProject\          本仓库
└── RAG\ue-rag\              https://github.com/LT916128944/ue-rag
```

## 这个仓库里有什么

- `AIAgentProject.uproject`：引擎关联 5.8。官方 Unreal MCP 与 Monolith 插件一次只开一个，跟 `RAG/ue-rag/.env` 的 `UE_EDITOR_BACKEND` 一致。
- `Source/AIAgentProject/`：游戏模块。巡逻敌人的 C++ 半边是 `ATestEnemy` 和 `UTestEnemyPatrolComponent`，发现玩家、追击、近战各留一个 `BlueprintImplementableEvent`，图由人接。旁边还有测试用的 Pawn、Character、GameMode、PlayerController。
- `Content/`：测试关卡和蓝图，含 `BP_Gun`、`BP_Sphere`、`BP_Character`、`BP_GameMode`、`Maps/L_NewTest`。
- `.cursor/`、`.codex/`：域技能和规则。技能正文以 `.codex/skills` 为源，当前 24 份，同步到 `.cursor/skills`。Codex 的始终生效短文在 `.codex/instructions/`，钩子在 `.codex/hooks.json`。发现技能前跑 `.codex/link_agents_skills.ps1`。
- `documents/`：功能设计。导出后进入 ue-rag 的检索。
- `Plan/`：过程记录，不进入检索。
- `start.bat`：转到 `RAG\ue-rag`，打开四条入口。

## 准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`。一次运行只连一个编辑器 MCP。本仓库 `.env.example` 正在用的是 `UE_EDITOR_BACKEND=monolith`，地址 `http://127.0.0.1:9316/mcp`，这时关掉官方 Unreal MCP 插件。改回 `official` 时反过来：开 `http://127.0.0.1:8000/mcp`，关掉 Monolith。两边同时在听，启动会被拒绝。
2. 按 [ue-rag 的 README](https://github.com/LT916128944/ue-rag) 准备 Python 3.12 虚拟环境、依赖和 `.env`。至少填 `DEEPSEEK_API_KEY`。密钥不要提交。复制 `.env.example` 得到的是这台机器正在用的值：模型 `deepseek-flash`，蓝图 / 材质 / UMG / 动画 / Niagara / AI / Monolith 写开关都开，保存和 PIE 的缺省也开。键没写时，代码缺省仍是 `official`、`deepseek-chat`、图开关关闭。
3. 第一次使用前的入库、工具索引和四条入口的差别，按 [`WORKFLOW.md`](WORKFLOW.md) 的「拉取之后先准备」。

准备好之后双击 `start.bat`：

| 选项 | 行为 |
| --- | --- |
| 1 普通对话 | 检索后回答。不改工程，不编译 |
| 2 单循环 | 一句话拆成步骤，每步走同一张图 |
| 3 多窗口 | 分层规划后分扇执行，再集成 |
| 4 续接 | 续接最近一个没跑完的多窗口 run |

菜单 2 和菜单 3 共用 `plan_windows`。写场景之后是否保存、玩法验收是否进 PIE，跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`（缺省都开）。任务里显式写 `save: false` / `pie: false` 才关。

## 现在做到哪

完成与否看 [`RAG/ue-rag/data/harness/feature_list.json`](../RAG/ue-rag/data/harness/feature_list.json)，不看模型正文。当前仍 pending 的三条：

- `patrol-presentation`：行为树、动画、受击三个 `BlueprintImplementableEvent` 仍由人绑。`.env` 里 `UE_ALLOW_AI=true` 不把这条改成 passed。
- `editor-transaction`：没有编辑器自己的 Transaction / Undo，属性回滚靠快照。
- `level-assembly-persist`：整句平台跳跃还不能标成功。金币、平台、门、进度中枢已在 `L_NewTest`，HUD 由 C++ `UPlatformHUDWidget` 驱动；`umg-hud-exec` 与 `level-assembly` 在清单里已是 done。

检索、改 C++、编译、读错误、再改、再测这条窄闭环可以对固定任务做编译和断言。目标句「一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人」的表现半边仍由人在编辑器里绑。

细节以 [`WORKFLOW.md`](WORKFLOW.md) 开头的「当前系统」为准，不要用 [`周计划.md`](周计划.md) 第 4.3 节那张开工时的缺口表判断当前仓库。

## 工作流怎么用

一句话结论：

- **编辑器一次只走一个后端**。本机 `.env.example` 是 Monolith（9316）。Cursor 在 `monolith` 时读 `ue-monolith-backend-skill`，用 discover / describe / exec；`official` 时才扫描官方工具。选错入口就停。
- **闭环仍是 `start.bat` 菜单 2/3**。DSH 是外壳：9 个 `mcp__uerag__*` 会转到当前锁定的后端，但不跑 LangGraph。改工程和编译回到 `run_windows.py`。
- **句子到窗口**仍是 L0 手写规则或已学技能 → L1 理解器 → L2 归地 → L3 回写。只读诊断句另由 `agent/diagnose.py` 归到唯一的 Automation 过滤器，收成已有的 `run_automation_test`，不往动作表加行。
- **蓝图在 Monolith 下同一目标最多 3 次**。第 3 次仍失败就停，不改连 8000。写开关关掉时次数是 0，步骤写入 `Plan/`。

### 先选入口

| 你要做的事 | 入口 | 一次花几次模型调用 |
| --- | --- | --- |
| 固定句式的窄任务（编译工程、编译一张蓝图、移动 Actor、拾取裁决、放 Actor） | `start.bat` 菜单 2/3 | L0 命中 0 次；弱命中 1 次理解器 |
| 新需求，要拆句、要改 C++、要摆场景 | `start.bat` 菜单 3（`run_windows.py`），先出任务书人审 | 1 次理解器 + 每扇图内 plan 若干次 |
| 只读研究、方案对比、报错定位（不改工程） | DeepSeek Harness（DSH）里的 `search_project` / `workflow` | 检索 0 次；研究模板 5 个子代理 |
| 单步编辑器操作 | 看 `UE_EDITOR_BACKEND`。`monolith` 用 `scripts/monolith_backend.py`；`official` 用扫描脚本。DSH 里仍是原来的 3 个编辑器工具，内部转到当前后端 | 每步 ≥1 次 |
| 续接没跑完的多窗口 run | `start.bat` 菜单 4 | 只跑没完成的扇 |
| 「看看有没有问题」这类只读诊断 | 菜单 3。归地成功就是一扇 `run_automation_test` | 诊断归地本身 0 次 |

### 一条 UE 需求的标准步骤

以「敌人发现玩家后播放追击动画」这类需求为例：

0. **预检**（DSH 外的终端，或双击 `..\RAG\ue-rag\shortcuts\dsh.bat`）：venv、`.env`、配置、技能、索引、当前后端的端口有一项不过就先修那一项。Monolith 时确认 9316 在听、8000 没在听。
1. **授权文件**：`RAG/ue-rag/data/dsh/session_task.json` 不存在时所有写操作被拒（fail-closed）。从 `session_task.example.json` 复制一份，第一次先保持 `allow_writes: []`；确实要写编辑器时再由人补上白名单、`save`、`pie`。
2. **开工读档**：跑一次 `python agent/resume_context.py`，或在 DSH 里调 `resume_context`。拿到 goal / phase / 下一条 pending / 最新交接的 unmet（≤200 字）。不要读 `WORKFLOW.md` 全文、不读 `ue_mcp_index.json`、不拉全量 schema。
3. **只读收敛范围**：`search_project(query, top_k≤5)` 查资产、C++、蓝图图、文档；排障用 `where={"doc_kind":"history"}` 或 `debug`，`top_k=1`。还不够就贴 [`../RAG/ue-rag/dsh/workflows/research.js`](../RAG/ue-rag/dsh/workflows/research.js) 到 DSH 的 `workflow` 工具。
4. **落盘任务书/计划**：需求会改工程时，按 `ue-cpp-plan-skill` 写 `Plan/YYYYMMDD-HHMMSS-<名字>.md` 或 `.cursor/memory/plans/`，等人确认。
5. **执行**：确认后回终端跑 `start.bat` 菜单 3。写编辑器或编译串行，只读裁决并行。窄任务走菜单 2。
6. **要证据**：`read_contract(run_id)` 看条款，再对未过且非 `manual` 的条款调 `verify(run_id, criterion_id)`。只有返回 `{ok:true, kind}` 的条款才算完成。
7. **人工部分**：行为树、动画、受击、仍标成 `manual` 的委托由人在编辑器里绑，步骤写进 `Plan/`。
8. **收尾**：多扇任务写交接 JSON，用 `python agent/dsh_cli.py handoff-validate` 和 `handoff-merge`。改了 C++ 或关卡再补当天 `.cursor/memory/任务记录/`。

### DSH 接上了什么

工具面仍是 9 个：`search_project`、`scan_editor_tools`、`get_editor_tool_schema`、`exec_editor_tool`、`read_contract`、`verify`、`read_handoff`、`window_state`、`resume_context`。后三个编辑器工具在 `server/dsh_bridge.py` 转到已锁定的后端，个数不增加。

写闸门仍在：`session_task.json` 缺失时 `exec_editor_tool` 回 `write refused`，并带 `policy`。`allow_writes` / `save` / `pie` 不是工具参数。

不接官方 830 个工具的全量 schema，也不把 Monolith 服务器配进会注入全部 schema 的客户端。DSH 不替代 `run_windows` / `judge`。

### token

结构没变：L0 命中是 0 次模型调用；只读检索是 0 次；新需求要过理解器，再按扇跑图内 `plan`。合计数字以 [`scoreboard.json`](../RAG/ue-rag/data/harness/scoreboard.json) 里现有的那一版为准：26 个任务、24 个 ok、每个成功任务约 126k prompt tokens。这份文件之后没有新的合计写进去。DSH 与菜单 2 的对照在 [`dsh_ab.json`](../RAG/ue-rag/data/harness/dsh_ab.json)，DSH 侧仍是 null，不能据此说 DSH 更省。

固定句式和窄任务可以压在理解器加一轮 `plan`。任意新需求会按扇叠加。表现条款里的 `manual` 仍要人绑。

### 红线

- 模型文本不改 `passed`；`manual` 条款只由人在 `Plan/` 里绑。
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
