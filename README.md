# AIAgentProject

UE 5.8 工程，给 AI Agent 一个可操作的编辑器和一份 C++ 游戏模块。检索、对话和执行闭环在旁边的 [ue-rag](https://github.com/LT916128944/ue-rag)。

| 文档 | 管什么 |
| --- | --- |
| 本文 | 仓库里有什么、怎么和 ue-rag 放在一起 |
| [`WORKFLOW.md`](WORKFLOW.md) | 已经接上的对话、检索、裁判和评测 |
| [`周计划.md`](周计划.md) | 学习路线、岗位对标、13 周计划 |
| [`AGENTS.md`](AGENTS.md) | Cursor 与 Codex 共用的工具路由和 Token 节流 |
| [`../RAG/ue-rag/dsh/README.md`](../RAG/ue-rag/dsh/README.md) | DeepSeek Harness 外壳：安装、9 个工具、写策略、排障 |

## 目录怎么放

`start.bat` 按相对路径找上一级的 `RAG\ue-rag`。两个仓库克隆之后要保持这个位置：

```text
AIWork\
├── AIAgentProject\          本仓库
└── RAG\ue-rag\              https://github.com/LT916128944/ue-rag
```

## 这个仓库里有什么

- `AIAgentProject.uproject`：引擎关联 5.8。已启用 `ModelContextProtocol`、`MCPClientToolset`、`AllToolsets`、`EditorToolset`。
- `Source/AIAgentProject/`：游戏模块。巡逻敌人的 C++ 半边是 `ATestEnemy` 和 `UTestEnemyPatrolComponent`，发现玩家、追击、近战各留一个 `BlueprintImplementableEvent`，图由人接。旁边还有测试用的 Pawn、Character、GameMode、PlayerController。
- `Content/`：测试关卡和蓝图，含 `BP_Gun`、`BP_Sphere`、`BP_Character`、`BP_GameMode`、`Maps/L_NewTest`。
- `.cursor/`、`.codex/`：域技能和规则。技能正文以 `.codex/skills` 为源，同步到 `.cursor/skills`。Codex 的始终生效短文在 `.codex/instructions/`，钩子在 `.codex/hooks.json`。发现技能前跑 `.codex/link_agents_skills.ps1`。
- `documents/`：功能设计。导出后进入 ue-rag 的检索。
- `Plan/`：过程记录，不进入检索。
- `start.bat`：转到 `RAG\ue-rag`，打开三条对话。

## 准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`。官方 Unreal MCP 需在 `http://127.0.0.1:8000/mcp`。编辑器没开时，写场景和改蓝图都会失败。
2. 按 [ue-rag 的 README](https://github.com/LT916128944/ue-rag) 准备 Python 3.12 虚拟环境、依赖和 `.env`。至少填 `DEEPSEEK_API_KEY`。密钥不要提交。
3. 第一次使用前的入库、工具索引和三条对话的差别，按 [`WORKFLOW.md`](WORKFLOW.md) 的「拉取之后先准备」。

准备好之后双击 `start.bat`：

| 选项 | 行为 |
| --- | --- |
| 1 普通对话 | 检索后回答。不改工程，不编译 |
| 2 单循环 | 一句话拆成步骤，每步走同一张图 |
| 3 多窗口 | 分层规划后分扇执行，再集成 |
| 4 续接 | 续接最近一个没跑完的多窗口 run |

蓝图、材质、UMG、动画蓝图、Niagara 的连线默认不改。要改蓝图图时，在 ue-rag 的 `.env` 里把 `UE_ALLOW_BLUEPRINT` 设为 true。写场景之后保存关卡、玩法验收进 PIE 都跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`（缺省都开），任务里显式写 `save: false` / `pie: false` 才关。

## 现在做到哪

检索、改 C++、编译、读错误、再改、再测这条窄闭环已经能对固定任务做编译和断言。目标句「一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人」还做不到：C++ 半边在，行为树、动画和受击表现仍由人在编辑器里绑。

细节和完成度以 [`WORKFLOW.md`](WORKFLOW.md) 为准，不要用 [`周计划.md`](周计划.md) 第 4.3 节那张开工时的缺口表判断当前仓库。

## 工作流怎么用（2026-10-23 现状）

一句话结论：

- **已经接上 DSH，但只当外壳**：9 个 `mcp__uerag__*` 工具 + 23 个技能 + 写闸门全部可用，DSH 负责对话、只读检索、研究、任务书；改工程和编译仍回 `start.bat` 菜单 2/3 的 `run_windows.py`。
- **可以低 token 跑，但要挑句式**：固定/已学句式 0 次模型调用；单个窄任务一万 token 以内；任意新需求做不到低，平均每个成功任务 126k prompt tokens，而且表现部分必须人绑。
- **最省的用法**：DSH 只读收敛范围（0–25k）→ 人确认计划 → 菜单 3 执行 → `read_contract` + `verify` 取证据。不要在 DSH 里一步步改工程。

### 先选入口

| 你要做的事 | 入口 | 一次花几次模型调用 |
| --- | --- | --- |
| 固定句式的窄任务（编译工程、编译一张蓝图、移动 Actor、拾取裁决、放 Actor） | `start.bat` 菜单 2/3 | L0 命中 0 次；弱命中 1 次理解器 |
| 新需求，要拆句、要改 C++、要摆场景 | `start.bat` 菜单 3（`run_windows.py`），先出任务书人审 | 1 次理解器 + 每扇图内 plan 若干次 |
| 只读研究、方案对比、报错定位（不改工程） | DeepSeek Harness（DSH）里的 `search_project` / `workflow` | 检索 0 次；研究模板 5 个子代理 |
| 单步编辑器操作（读属性、编译已有蓝图、移动一个 Actor） | DSH 的 9 个 `mcp__uerag__*` 工具 | 每步 ≥1 次 |
| 续接没跑完的多窗口 run | `start.bat` 菜单 4 | 只跑没完成的扇 |

DSH 只是外壳：检索、执行、裁判、回滚仍是 `RAG/ue-rag` 内核。要动编辑器、要跑 C++ 闭环，最终都回到 `run_windows.py`，DSH 不跑那张 LangGraph。

### 一条 UE 需求的标准步骤（低 token 路线）

以「敌人发现玩家后播放追击动画」这类需求为例：

0. **预检**（DSH 外的终端，或双击 `..\RAG\ue-rag\shortcuts\dsh.bat`）：venv、`.env`、配置、技能、索引、编辑器连通有一项不过就先修那一项。
1. **授权文件**：`RAG/ue-rag/data/dsh/session_task.json` 不存在时所有写操作被拒（fail-closed）。从 `RAG\ue-rag\data\dsh\session_task.example.json` 复制一份，第一次先保持 `allow_writes: []`；确实要写编辑器时再由人补上白名单、`save`、`pie`。
2. **开工读档**：DSH 里调一次 `resume_context`，拿到 goal / phase / 下一条 pending / 最新交接的 unmet（≤200 字）。不要读 `WORKFLOW.md` 全文、不读 `ue_mcp_index.json`、不拉全量 schema。
3. **只读收敛范围**：`search_project(query, top_k≤5)` 查资产、C++、蓝图图、文档；排障用 `where={"doc_kind":"history"}` 或 `debug`，`top_k=1`。还不够就贴 [`../RAG/ue-rag/dsh/workflows/research.js`](../RAG/ue-rag/dsh/workflows/research.js) 到 DSH 的 `workflow` 工具，产出 `data/briefs/<ts>-<slug>.md`。
4. **落盘任务书/计划**：需求会改工程时，按 `ue-cpp-plan-skill` 写 `Plan/YYYYMMDD-HHMMSS-<名字>.md` 或 `.cursor/memory/plans/`，等人确认。
5. **执行**：确认后回终端跑 `start.bat` 菜单 3（`python agent/run_windows.py`），把这句话交给它拆扇；每扇一次 `run_task`，写编辑器/编译串行、只读裁决并行。窄任务走菜单 2 更快。
6. **要证据**：`read_contract(run_id)` 看条款，再对未过且非 `manual` 的条款调 `verify(run_id, criterion_id)`。只有返回 `{ok:true, kind}` 的条款才算完成；模型正文里的「已完成」不改 `passed`。
7. **人工部分**：行为树、动画、受击、UI 委托这类 `manual` 条款永远 false，由人在编辑器里绑，步骤写进 `Plan/`。
8. **收尾**：多扇任务写交接 JSON，`python agent/dsh_cli.py handoff-validate --file ...` 校验、`handoff-merge --file ... --run ...` 合并；改了 C++ 或关卡再补当天 `.cursor/memory/任务记录/`。

### DSH 现在接上了什么、没接什么

已经接上（2026-10-23 本机实测）：

- profile patch 三条（`mcp-uerag` + `skill-filesystem` + `tool-skill`），`E:\UEProject\AIWork\.dsh\skills` 已联接到 `.agents\skills`，23 个技能可发现；`check_dsh_config.py` 打印 `dsh-config ok (3 entries, 23 skills)`，`check_dsh_bridge.py` 打印 `dsh-bridge ok`。
- 工具面恰好 9 个 `mcp__uerag__*`：`search_project`、`scan_editor_tools`、`get_editor_tool_schema`、`exec_editor_tool`、`read_contract`、`verify`、`read_handoff`、`window_state`、`resume_context`。本轮实际调通 5 个：`resume_context`（回 goal/phase/pending/unmet）、`search_project`（`BP_Coin` 命中 `/Game/BP_Coin`）、`scan_editor_tools`（出 3 张卡片、无 schema）、`read_contract`（读 `dsh-compile-selftest-20261023` 的 `c-build` = passed true）、`exec_editor_tool`。
- 写闸门生效：`session_task.json` 当前不存在，`exec_editor_tool(set_actor_transform)` 回 `{"ok": false, "error": "write refused"}` 并带 `policy`（`save:false`、`pie:false`、`allow_writes:[]`、五个 `UE_ALLOW_*` 当时的值）。这是设计行为，不是故障。

还没接、也不打算接：

- 不接官方 UE MCP 的 830 个工具（DSH 会把 server 全部 schema 注入每个请求），DSH 里也不拉全量 schema。
- DSH 不跑闭环、不替代 `run_windows` / `judge` / `editor_exec`；`allow_writes` / `save` / `pie` 不是工具参数，只由人改文件。
- DSH 侧 token 还没有实测数（[`dsh_ab.json`](../RAG/ue-rag/data/harness/dsh_ab.json) 三项仍是 null），所以现在不能宣称 DSH 比菜单 2 更省。

### token 基线（实测，不是估算）

| 环节 | 数值 | 来源 |
| --- | --- | --- |
| L0 手写规则 / 已提升技能命中 | 0 次模型调用 | 事件流 `run_start.source=rules` |
| L1 理解器 | p50 995 / max 1581 prompt tokens | [`dsh_baseline.json`](../RAG/ue-rag/data/harness/dsh_baseline.json)（n=48） |
| 老规划者 | p50 1022 prompt tokens | 同上（n=18） |
| 图内 `plan` 节点 | p50 8008 / max 12435 prompt tokens | 同上（n=133） |
| 每个成功任务平均 | 126k prompt tokens（26 任务 / 24 ok） | [`scoreboard.json`](../RAG/ue-rag/data/harness/scoreboard.json) |
| DSH 会话 | 未测；计划预算单任务研究 ≤25k | [`dsh_ab.json`](../RAG/ue-rag/data/harness/dsh_ab.json)、[`Plan/20261023-1520-DeepSeekHarness深度绑定.md`](Plan/20261023-1520-DeepSeekHarness深度绑定.md) 第 8 节 |

### 现在能做到多低

- **能低**：固定句式和已学句式是 0 次调用；只读检索是 0 次；窄任务（编译工程、编译一张蓝图、移动一个 Actor）一般只需 1 次理解器（约 1k）+ 1 次图内 `plan`（约 8k），合计一万以内；DSH 只读研究一次压在 25k 以内。
- **不能低**：任意新需求全自动。新需求至少要过 1 次理解器，再按扇跑图内 `plan`（p50 8k/轮）；一扇 3–5 轮就是 24–40k，多扇叠加到十万量级——`scoreboard.json` 上每个成功任务平均 126k prompt tokens。表现部分本来就要人绑。
- **所以**：先让 DSH 只读收敛范围（便宜、可并行），再把定好的一句话交给 `run_windows` 执行（贵但可验证）。不要在 DSH 里逐步改工程，那会把常驻前缀乘上每一步。

### 红线

- 模型文本不改 `passed`；`manual` 条款只由人在 `Plan/` 里绑。
- 不用 DSH 的 read/write 直接改 `.uasset` / `.umap` / `Source/`；UE 写只经 `mcp__uerag__*` 网关。
- DSH 会话不写 `allow_writes`、不写 `save` / `pie`、不写 `.env`。
- 入库、导出、索引刷新（写 `data/chroma`）放在 DSH 外的终端跑，别在 DSH 会话里跑 `pipelines/` 或 `shortcuts\*.bat`。

### 排障

| 现象 | 处理 |
| --- | --- |
| `write refused` + `policy` | 先看 `data/dsh/session_task.json` 是否存在、`allow_writes` 是否含该工具；不要换工具重试 |
| 预检报 `chroma` readonly，或 shell 写文件报 Access denied | 是 DSH shell 以低完整性级别跑、写不进 `data/chroma`，不是仓库 ACL 坏了。把预检和入库放到 DSH 外的普通终端跑即可（实测普通终端 `chroma — ue_assets=242`、`preflight ok`） |
| `UE MCP :8000` 不通、`UnrealEditor` 未运行 | 编辑器没开，先打开 `AIAgentProject.uproject`；写场景、改蓝图都要它。本轮实测就是关着编辑器，所以预检这两行是 `miss`（非 critical），`preflight ok` 仍打印 |
| 扫描两轮仍对不上工具 | 停，只说「未知 MCP 工具全路径和 JSON 参数」，不回退全量 schema |

校验命令（工作目录 `RAG/ue-rag`，Python 用该目录 `.venv`）：

```powershell
python shortcuts/dsh_preflight.py      # venv / .env / 配置 / 索引 / 编辑器连通
python eval/check_dsh_config.py        # dsh-config ok
python eval/check_dsh_bridge.py        # dsh-bridge ok
python eval/check_dsh_contract.py      # dsh-contract ok
python eval/check_dsh_lock.py          # dsh-lock ok
python eval/check_dsh_costs.py         # dsh-costs ok
python scripts/import_dsh_costs.py     # DSH 导出的会话用量 → data/costs.jsonl
python eval/dsh_ab.py                  # 三条固定任务的 A/B 表
```
