---
id: dsh-bind-20261023
priority: 90
pre_authorized: true
ask_user: false
stop_to_confirm: false
session: 1
depends_on: []
last_updated: 2026-10-23
status: active
owner: 侑
---

# DeepSeek Harness 深度绑定：分步落地计划

把 DSH 接成「对话 / 研究 / 编排外壳」，把 ue-rag 保留为「检索 / 执行 / 裁判 / 回滚内核」。**不是**用 DSH 换掉 LangGraph，也不是让 DSH 的模型替代裁判。

一句话：

> DSH 只做入口和只读并行；所有 UE 写操作仍走 `core/editor_exec.py`，所有 `passed` 仍由 `core/judge.py` 的裁判改写。DSH 拿不到"宣布成功"的权力，也拿不到"放宽 allow_writes"的权力。

本计划批准后按 P0 → P6 顺序执行，每阶段有独立完成标记和回滚点。**交给 Cursor 执行时，用 `Plan/DeepSeekHarness绑定执行/` 下的分卡**（一卡一会话，卡内含允许写入、步骤、验收命令与人工门）；本文件作为设计与验收总纲，不逐卡重读。前置分析见 [`../../DeepSeekHarness融合分析.md`](../../DeepSeekHarness融合分析.md)、[`../../.analysis/ue-system-analysis.md`](../../.analysis/ue-system-analysis.md)、[`../../.analysis/dsh-capabilities.md`](../../.analysis/dsh-capabilities.md)（实施时不要重读底稿全文）。

---

## 0. 本计划怎么用

1. 先在会话里按「批准清单」（第 13 节）批准本文件，把 `pre_authorized` 改成 `true`。
2. 每个会话只做一个阶段。开工只读：本文件、`AGENTS.md`、`documents/harness/overview.md`、`data/harness/feature_list.json` 里相关条目；不要读 `WORKFLOW.md` 全文、不要读 `ue_mcp_index.json`。
3. 每个阶段的「完成标记」通过后才能进下一阶段；失败最多修一次，第二次失败把命令原文写进文末「执行记录」并停。
4. Python 一律用 `E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe`，工作目录 `RAG/ue-rag`。
5. DSH 侧改动只写 `$DSH_HOME`（默认 `C:\Users\X\.dsh`），不把 DSH 安装目录当仓库改。

---

## 1. 结论与目标

### 1.1 三个目标

| 目标 | 判定 |
| --- | --- |
| 入口统一 | 一句 UE 需求可以在 DSH 里完成「查 → 读 → 计划 → 调工具 → 拿裁判证据」的全过程，不再必须在 Cursor 里按 AGENTS.md 说话 |
| 能力不降 | DSH 路径下的写操作仍被 `editor_exec` 拒绝链拦住；`passed` 仍只由 `judge.ok && kind` 改写；830 个 schema 仍不进任何模型上下文 |
| 成本可控 | 固定句式与执行任务继续走菜单 2/3（L0 零调用、L1 ~1k）；DSH 只承接研究/编排，单任务 prompt token 有上限、有记分 |

### 1.2 不追求的事

- 不追求「一个 DSH 替代全部」：那会丢掉裁判、RAG、回滚与快照。
- 不追求「DSH 双活替代 Cursor」：Cursor 继续保留 `.cursor/rules` 与 `.cursor/skills` 镜像；DSH 是第二条入口。
- 不追求「DSH 自己跑闭环」：闭环（`run_windows`）继续是唯一业务图执行者；DSH 只生成任务书与只读扇。

---

## 2. 不动清单（全程约束）

- 一张 LangGraph：`retrieve → plan → execute → build → test → observe → fix`。不加节点、不改边、不加第二张业务图。
- `core/editor_exec.py` 仍是**唯一** UE 编辑器出口；DSH 侧不新增第二条写通道。
- `passed` 只由 `core/judge.py` 的裁判或离线检查改写；`manual` 永远 false；模型正文里的「已完成」不改状态。
- 不换供应商、不改 `base_url`；不把 `--query` 接成 DSH 每句话的默认动作。
- 不保存关卡、不 PIE、不改蓝图 Graph，除非任务策略/`UE_ALLOW_*` 显式允许；**本计划不修改 `.env` 里任何开关的值**。
- 不把官方 830 个 MCP 工具接进 DSH（DSH 会把 server 全部工具 schema 每请求注入）。
- 不 Docker、不 Loki/Prometheus、不做常驻观测服务。
- 不改 `Source/`、`Content/`、蓝图、关卡；不 `git commit` / `git push` / 改 git config。
- 不改 `feature_list.json` 已有条目的 `passed`；只追加本计划的 `pending` 行，`passed` 由对应离线检查改。

---

## 3. 现状与缺口

| 层 | DSH 原生 | 你现在 | 本计划补 |
| --- | --- | --- | --- |
| 规则注入 | AGENTS.md 分层、64KB 预算、fs touch 刷新 | `AGENTS.md` + `.mdc` 四条 alwaysApply | 根 `AGENTS.md` 指针 + 项目内 DSH 段（P1） |
| 技能 | `.dsh/skills` / `.agents/skills` / custom 根，`SKILL.md` 按需加载 | 19 个 `.codex/skills`（`.agents/skills` 已联接） | 1 个 DSH 会话技能 + 3 个 DSH 操作技能（P1/P3） |
| 工具 | MCP stdio/HTTP，工具名 `mcp__server__tool` | `server.py`（2 工具）、`contract_server.py`（6 工具）、830 工具脚本闸门 | 薄 MCP 网关 9 工具（P2） |
| 执行/裁判 | 无 | `editor_exec` + `judge` + `harness` | 经网关暴露，DSH 只调用不实现（P2/P3） |
| 写闸门 | 文件级写前读；MCP 进程不受沙箱管 | `editor_exec` 七步拒绝链 + 快照 | 拒绝链单源化 `core/graph_write_policy.py`（P2） |
| 合同 | 无 | `criteria[]` + `apply_judge` | `read_contract` / `verify` 工具（P2/P3） |
| 编排 | subagent / workflow / agent-team | `run_windows` + 扇间交接 | DSH 只读扇 + handoff 校验（P4） |
| 观测 | token-meter、会话日志，无成本账本 | `costs.jsonl`、scoreboard | DSH 成本导入脚本（P5） |

---

## 4. `.cursor` 资产评估（对 Harness 有什么帮助）

| 资产 | 内容 | 对 DSH 的价值 | 处置 |
| --- | --- | --- | --- |
| `.cursor/rules/*.mdc`（4 份，始终生效） | `ue-rag-route` / `ue-cpp-plan` / `ue-harness-session` / `ue-history`，共约 3.3KB | 这四份是「会话开工协议 + 工具路由 + 计划门 + 排障门」，**DSH 不读 `.mdc`**，但内容正是 DSH 会话需要的 | 不复制全文。以 `.codex/instructions/` 四份对等的纯 Markdown 为素材，压成一个 `ue-dsh-session-skill`（≤40 行）；`.mdc` 保留给 Cursor（P1） |
| `.cursor/hooks/refuse_graph_writes.py` | 图写拦截 `decide()`（蓝图/材质/UMG/动画/Niagara 开关），119 行，纯函数可测 | **这是 `.cursor` 里最值钱的代码**：DSH 没有配置式 hook，但这段判定可以原样复用 | 提为 `core/graph_write_policy.py`，Cursor/Codex hook 与新 MCP 网关三处共用（P2） |
| `.cursor/hooks.json` | `beforeShellExecution` 调上面脚本，`failClosed: false` | Cursor 侧是软拦截（fail-open）；DSH 侧必须**硬拦截**（fail-closed） | DSH 网关内调用同一函数，读不到 `.env` 也拒绝（P2） |
| `.codex/hooks/refuse_graph_writes.py` | 与 Cursor 版重复的第二份判定 | 两份会漂移；DSH 接入后变三份 | P2 一起改成薄包装，只 import 共享模块 |
| `.codex/hooks/session_start.py` | 注入四份 Codex 短文 | DSH 用 AGENTS.md 分层替代，不需要照搬 | 不动 |
| `.cursor/memory/conventions.md` | 34 行「已沉淀」UE 事实（长度单位、Toolset、BindKey、Widget 父类、拾取判定等） | 这是**领域知识金矿**；已被 `export_docs.py` 切块进 RAG（`ue_doc`） | 不搬进 AGENTS.md。DSH 用 `search_project(where={"source_type":"ue_doc"})` 或 `ue-history` 技能按需取（P1 技能里写死路径） |
| `.cursor/memory/plans/*.md`、`任务记录/*.md` | 人读计划与任务记录（6+6 份） | 交接的 `next` 已读「未完成标题」；DSH 不该整份读 | 沿用现有规则：只读当天任务记录的未完成标题（P3 handoff 技能） |
| `.cursor/skills/*`（19 个镜像） | 与 `.codex/skills` 同名同内容 | **不要**把 `.cursor/skills` 加进 DSH `customSkillDirs`，会重复发现、目录变长 | DSH 只扫 `.codex/skills`（经 `.agents/skills` 联接）（P1） |
| `.cursor/AGENTS.md` | 已迁移指针（205B） | 被 DSH 触达时会多注入一次 | 不动；DSH 会话里避免读该目录 |
| `.cursor/rules` 的 `alwaysApply: true` | Cursor 语义 | DSH 没有 alwaysApply，只有 AGENTS.md 常驻 + 技能按需 | 必须遵守的放 AGENTS.md；可执行的靠网关硬拦（P1/P2） |

结论：`.cursor` 对 DSH 的帮助集中在两点——**四份规则是现成的会话技能素材；`refuse_graph_writes.py` 的 `decide()` 是现成的写闸门实现**。其余记忆资产继续留在检索层，不搬进上下文。

---

## 5. 目标架构

```text
用户
 │
 ├─ 菜单 1/2/3（不变）────────────► run_task / run_windows（LangGraph）
 │                                     ├─ L0/L1 规划
 │                                     ├─ editor_exec（唯一写闸门）
 │                                     ├─ judge（唯一裁判）
 │                                     └─ snapshots / handoff / ledger
 │
 └─ DSH（新入口）── AGENTS.md 分层 + 技能按需加载
        │
        ├─ 只读：mcp__uerag__search_project / scan_editor_tools / get_editor_tool_schema
        │        read_contract / read_handoff / resume_context
        ├─ 写：  mcp__uerag__exec_editor_tool ──► editor_exec（策略来自 data/dsh/session_task.json）
        │                                          └─ graph_write_policy（fail-closed）
        ├─ 证：  mcp__uerag__verify ──► agent/dsh_cli.py ──► core.judge + harness.apply_judge
        └─ 编排：workflow/subagent 只读扇 ──► data/briefs/、data/handoffs/
```

数据流原则：**DSH 不直接写工程文件**（除 AGENTS/技能/文档由人写），工程写入一律经 MCP 网关 → `editor_exec`；**DSH 不直接写 `passed`**，只能调用 `verify` 让裁判写。

---

## 6. 分阶段

### 阶段 P0 · 冻结与基线（0.5 天）

**目标**：在执行前留下可回滚、可对比的现场。

步骤：

1. 记录两个仓库起点：`git -C AIAgentProject status --short`、`git -C RAG/ue-rag status --short`，写进 `data/dsh/baseline-git.txt`（只读，不提交）。
2. 备份但不打印内容（**备份放仓库外**，避免 `.env` 副本被误提交）：
   - `RAG/ue-rag/.env` → `C:\Users\X\.dsh\backups\ue-rag\env-<ts>.txt`
   - `C:\Users\X\.dsh\profiles\desktop\cordis.patch.yml` → `C:\Users\X\.dsh\backups\ue-rag\cordis.patch.<ts>.yml`
3. 跑基线检查，输出存 `data/dsh/baseline-checks.txt`：

```powershell
python eval/check_routes.py
python eval/check_harness.py
python eval/check_info_layer.py
python eval/check_architecture.py
python eval/check_schema.py
python eval/check_risk.py
python eval/check_automation.py
python eval/check_scene_gate.py
python eval/check_distill.py
python eval/check_upgrade.py
```

4. 新增 `eval/dsh_baseline.py`（只读 `data/costs.jsonl`、`data/harness/scoreboard.json`、`data/traces/*.jsonl`），产出 `data/harness/dsh_baseline.json`：
   `{callers: {understand: {n, p50, max}, planner: ..., plan: ...}, scoreboard: {...}, generated_at}`。不调模型、不改任何状态。

完成标记：九条检查输出与今天一致（已知 CI 硬编码的两条只在本机跑，记录即可）；`dsh_baseline.json` 四个 caller 有数。
新增 feature id：无。回滚：删 `data/dsh/` 与 `data/harness/dsh_baseline.json`。

---

### 阶段 P1 · 静态绑定：DSH 配置 + 规则/技能（1 天）

**目标**：DSH 启动后就能看见你的规则、19 个技能、项目检索与契约工具；此时还不引入新代码。

#### P1.1 仓库内配置模板

新增 `RAG/ue-rag/dsh/cordis.patch.yml`（仓库内唯一事实源，复制到 DSH profile）与 `RAG/ue-rag/dsh/README.md`（安装/校验/回滚三步）。

模板内容 = 最终态的雏形（P1 先接两个现有 server，P2 再换成网关）：

```yaml
- id: mcp-ue-rag
  name: '@deepseek-ai/dsh-mcp-client'
  config:
    serverName: uerag
    transport: stdio
    command: 'E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe'
    args: ['E:\UEProject\AIWork\RAG\ue-rag\server\server.py']
    cwd: 'E:\UEProject\AIWork\RAG\ue-rag'
    toolCallTimeoutMs: 120000

- id: mcp-ue-contract
  name: '@deepseek-ai/dsh-mcp-client'
  config:
    serverName: uec
    transport: stdio
    command: 'E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe'
    args: ['E:\UEProject\AIWork\RAG\ue-rag\server\contract_server.py']
    cwd: 'E:\UEProject\AIWork\RAG\ue-rag'
    toolCallTimeoutMs: 320000

- id: skill-filesystem
  name: '@deepseek-ai/dsh-skill-filesystem'
  config:
    customSkillDirs:
      - 'E:\UEProject\AIWork\AIAgentProject\.agents\skills'
    watch: true

- id: tool-skill
  name: '@deepseek-ai/dsh-tool-skill'
  config:
    catalogDescriptionMaxLength: 240
```

安装：把上面条目**追加**到 `C:\Users\X\.dsh\profiles\desktop\cordis.patch.yml`（patch 是替换整条 config，不是深合并；只写需要覆盖的字段，其余走默认）。

#### P1.2 根 AGENTS.md 指针

新增 `E:\UEProject\AIWork\AGENTS.md`（≤25 行，工作区根；`AIWork` 没有 `.git`，DSH 以 cwd 为项目根，需要它做 Tier 1）：

- 两个仓库的地图（AIAgentProject 工程 + RAG/ue-rag 内核）；
- 「UE 写操作只经 DSH MCP `mcp__uerag__*`，不直接用 read/write 改 `.uasset/.umap`」；
- 「`passed` 只由裁判改；描述完成要引用 `id` + `evidence`」；
- 「开工先调 `resume_context`，不读长文」；
- 技能根、DSH 配置模板的位置。

#### P1.3 项目 AGENTS.md 增补

在 `AIAgentProject/AGENTS.md` 末尾追加一节「DeepSeek Harness（2026-10-23 起）」（≤15 行）：DSH 入口、MCP 工具名前缀、`data/dsh/session_task.json` 是唯一写策略来源、链到 `RAG/ue-rag/dsh/README.md`。不改九条铁律，不重写全文。

#### P1.4 第一个 DSH 技能

新增 `.codex/skills/ue-dsh-session-skill/SKILL.md`（并同步到 `.cursor/skills/` 同名目录，遵守现有维护规则）。frontmatter 只写 `name` + `description`（≤240 字）；正文 ≤40 行，素材取 `.codex/instructions/` 四份短文（与 `.cursor/rules` 等价），压进要点：

1. 开工：调 `mcp__uerag__resume_context`（P2 提供）或读 `progress.json` + `feature_list.json` 第一条 pending；
2. 路由：编辑器操作先 `scan_editor_tools`（卡片、无 schema）→ `get_editor_tool_schema`（单个）→ `exec_editor_tool`；**禁止**在 DSH 里直连 830 工具；
3. 计划门：要改 C++/蓝图/关卡前，先把计划追加到 `.cursor/memory/plans/YYYY-MM-DD.md`，等确认；
4. 写闸门：`exec_editor_tool` 的 `allow_writes/save/pie` 只从 `data/dsh/session_task.json` 读，模型参数里没有这些字段；
5. 证据：任何「完成」前先 `read_contract` + `verify`，引用 `{ok, kind}`；`manual` 条款保持 false。

#### P1.5 预检脚本

- 新增 `RAG/ue-rag/shortcuts/dsh_preflight.py`：检查 venv、`.env`、DSH profile patch 是否含两条 MCP、`data/chroma` collection 计数、`127.0.0.1:8000` 是否可连、`UnrealEditor.exe` 是否在跑；输出 ≤40 行，退出码 0/1。
- 新增 `RAG/ue-rag/shortcuts/dsh.bat`：打印工作区与 preflight 结果，提示「在 DSH 中打开 `E:\UEProject\AIWork`」。

#### P1.6 离线检查

新增 `eval/check_dsh_config.py`（不连 DSH、不连编辑器）：

- PyYAML 解析模板（PyYAML 已在 requirements），断言 entry id 唯一、`serverName` 唯一、`transport` 合法、路径存在、无 830 工具 server；
- 断言 `skill-filesystem.customSkillDirs` 指向 `.agents/skills`，且该目录下 `*/SKILL.md` 数量 ≥19、frontmatter 合法；
- 断言 `catalogDescriptionMaxLength ≤ 240`；
- 打印 `dsh-config ok`。

完成标记：`python eval/check_dsh_config.py` 打印 `dsh-config ok`；DSH 里技能列表出现 `ue-dsh-session-skill`；工具列表出现 `mcp__uerag__search_ue_project` 与 `mcp__uec__query_actor` 等（6 个）；`shortcuts\dsh.bat` 预检输出正常。
feature id：`dsh-config`。
回滚：删 profile 里新增条目（用 P0 备份恢复）、删根 AGENTS.md 与技能目录。

---

### 阶段 P2 · 写闸门单源化 + 薄 MCP 网关（3 天）

**目标**：把「写闸门 + 裁判证据」搬到一个 DSH 只能调用、不能改写策略的 MCP server；工具总数 ≤9，不把 830 工具带进上下文。

#### P2.1 判定函数单源化

- 新增 `core/graph_write_policy.py`：从 `.cursor/hooks/refuse_graph_writes.py` 迁入 `BP_WRITES`、`_ENV`、`env_flag`、`blueprint_allowed`、`domain_allows`、`_flag`、`_is_read`、`_domain_key`、`decide`，**行为逐字节保持不变**。
- `.cursor/hooks/refuse_graph_writes.py`、`.codex/hooks/refuse_graph_writes.py` 改为薄包装：算 repo 根 → `sys.path` 插入 `RAG/ue-rag` → `from core.graph_write_policy import decide, blueprint_allowed, domain_allows` → 原有 stdin/stdout 协议不变。
- `eval/check_routes.py` 的 hook 拒绝断言改为 import 新模块；新增一条：`decide("python call_mcp_tool.py --tool write_graph_dsl ...", False, {})` 返回 deny，`UE_ALLOW_BLUEPRINT` 缺失时仍 deny（fail-closed）。

#### P2.2 快照函数独立

- 新增 `agent/snapshot.py`：把 `run_windows._snapshot_source` / `_restore_source`（及其私有依赖）迁入，公开为 `snapshot_source(run_id, window_id)` / `restore_source(run_id, window_id, snap)`。
- `agent/run_windows.py` 改为 `from agent.snapshot import ...`，行为不变；`check_upgrade.py` 的快照用例仍通过。

#### P2.3 证据 CLI（agent 层）

新增 `agent/dsh_cli.py`（只被 MCP 网关以子进程调用；也可以人手跑），所有子命令输出单行 JSON、不打印密钥：

| 子命令 | 作用 | 约束 |
| --- | --- | --- |
| `resume-line` | 转出 `resume_context.build_line(...)` | 只读 |
| `contract-status --run <id>` | 读 `data/runs/**/<id>.json` 的 `criteria`，调 `harness.contract_status` | 只读 |
| `verify --run <id> --criterion <cid> [--dry-run]` | 由 `criterion.evidence_kind` + 绑定窗口的 `window.task.verify` 取 spec，调 `core.judge.judge`，再 `harness.apply_judge`；原子写回 run JSON（临时文件+replace，持 `data/locks/run-<id>.lock`） | 无匹配 spec 就失败，不猜；`manual` 永远 false；不执行写场景 |
| `handoff --run <id> [--window <wid>]` | 读 `data/handoffs/<run>/<wid>.json` | 只读 |
| `handoff-validate --file <path>` | 校验 DSH 产出的 handoff：字段、`unmet`≤8、`unmet_total/omitted`、不含工具正文（含 `read_source` 则失败） | 只读 |
| `window-state snapshot\|restore --run <id> --window <wid>` | 调 `agent.snapshot` | 只重建/恢复源码与 props，不保存关卡 |

#### P2.4 MCP 网关

新增 `server/dsh_bridge.py`（stdio，`MCPServer("ue-rag-dsh")`），边界只 import `config` / `core`，agent 层一律走 `agent/dsh_cli.py` 子进程（保持 `check_architecture.py` 的依赖方向，不新增白名单）。

工具面（最终态 9 个，全部写死 schema，模型无法传 `allow_writes/save/pie`）：

1. `search_project(query, top_k=5, where=None)` → `core.rag_core.retrieve`
2. `scan_editor_tools(query, top_k=8)` → 复用 `scripts/scan_mcp_tools.py` 的卡片逻辑（subprocess，stdout 截断）
3. `get_editor_tool_schema(full_name)` → 同上 `--schema`
4. `exec_editor_tool(toolset, tool, args)` → 读 `data/dsh/session_task.json` 组装 task，调 `core.editor_exec.exec_tool`；`risk.tier` 为 mid/high 或 save/PIE/写前缀时先取写锁
5. `read_contract(run_id)` → agent/dsh_cli `contract-status`
6. `verify(run_id, criterion_id)` → agent/dsh_cli `verify`
7. `read_handoff(run_id, window_id=None)` → agent/dsh_cli `handoff`
8. `window_state(action, run_id, window_id)` → agent/dsh_cli `window-state`（action 只允许 `snapshot`/`restore`）
9. `resume_context()` → agent/dsh_cli `resume-line`

网关内部规则（全部硬编码，不读模型参数）：

- **策略来源**：`data/dsh/session_task.json`，人写；结构 `{allow_writes:[], save:false, pie:false, allow_blueprint_ref:"", note:""}`。文件缺失 → 写操作全部拒绝（fail-closed）。
- **写锁**：`data/locks/editor.lock`（pid + ts）；stale >900s 视为失效；取不到锁返回 `{ok:false, error:"editor busy"}`。
- **大输出**：原始结果写 `data/spill/<run>-<tool>-<ts>.json`，内联 ≤1KB（`{ok, summary, artifacts:[path], policy}`）。
- **运行态目录**：`data/dsh/`、`data/locks/`、`data/spill/` 加入 `RAG/ue-rag/.gitignore`；`session_task.json` 只留模板 `data/dsh/session_task.example.json`（可提交），真实文件不提交。
- **拒绝带 policy**：复用 `editor_exec._refused` 的 `policy_snapshot`，DSH 可见当时开关。
- **不出网**：网关不调用任何 LLM；`search_project` 的 embedding 本地跑。

#### P2.5 检查

新增 `eval/check_dsh_bridge.py`（mock 模式，不连编辑器、不调模型）：

- `scan_editor_tools` 返回卡片且**不含** `input_schema`；
- `exec_editor_tool` 在 `session_task.json` 为空时拒绝 `set_actor_transform`（`write refused` + `policy`）；`query_actor` 在 mock 下允许；
- `verify`：judge kind 不匹配 → `passed` 不变；匹配且 ok → `passed: true`；`manual` 永远 false；model 文本 `"已完成"` 不改；
- `read_contract` 读取 fixture run JSON，`contract_status` 与 `harness` 一致；
- 并发两次写：第二次 `editor busy`；
- 网关 schema 里没有 `allow_writes/save/pie` 参数；
- 打印 `dsh-bridge ok`。

#### P2.6 切换到最终 patch

把 `dsh/cordis.patch.yml` 里的 `mcp-ue-rag`/`mcp-ue-contract` 两条替换为一条：

```yaml
- id: mcp-uerag
  name: '@deepseek-ai/dsh-mcp-client'
  config:
    serverName: uerag
    transport: stdio
    command: 'E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe'
    args: ['E:\UEProject\AIWork\RAG\ue-rag\server\dsh_bridge.py']
    cwd: 'E:\UEProject\AIWork\RAG\ue-rag'
    toolCallTimeoutMs: 320000
```

完成标记：`check_dsh_bridge.py` 打印 `dsh-bridge ok`；`check_architecture.py` 仍 `architecture ok`；`check_routes.py` 仍 `routes ok`；DSH 工具列表恰好 9 个 `mcp__uerag__*`，搜索不到 `write_graph_dsl` 之外的新写入口。
feature id：`dsh-bridge`、`dsh-graph-write-policy`。
回滚：profile 恢复 P0 备份；代码保留不影响现有入口。

---

### 阶段 P3 · 技能与合同：让 DSH「先证后说」（3 天）

**目标**：DSH 里模型不再能凭嘴说完成；合同状态与裁判证据成为它唯一可引用的结论。

#### P3.1 三个 DSH 技能（源在 `.codex/skills`，同步 `.cursor/skills`）

| 技能 | 触发 | 正文要点（≤50 行） |
| --- | --- | --- |
| `ue-dsh-route-skill` | 要动编辑器/编译 | 先扫卡片→单个 schema→exec；同一任务 ≤8 次调用；失败原文交人；`data/dsh/session_task.json` 是策略源，模型不能改 |
| `ue-dsh-verify-skill` | 任何「完成/修好/通过」之前 | `read_contract(run)` → 对每条未过条款 `verify(run, cid)`；引用 `{ok,kind}`；`manual` 不碰；连续 2 条 verify 失败就停并报 unmet |
| `ue-dsh-handoff-skill` | 多步骤/多扇任务收尾 | 写 `data/handoffs/<run>/<win>.json`；字段 `did/evidence/unmet/unmet_total/unmet_omitted/next/artifacts`；不带工具正文；用 `handoff-validate` 自检 |

#### P3.2 AGENTS.md 增补合同三句话

在 P1.3 那一节后补三句（≤6 行）：

1. 「说完成之前先 `verify`。没有 `{ok:true, kind}` 的条款就是没完成。」
2. 「`manual` 条款永远不由模型改；它由人在 `Plan/` 里绑。」
3. 「DSH 会话不写 `allow_writes`、不写 `save/pie`、不写 `.env`。」

#### P3.3 verify 的映射规则（实现细节）

- `criterion.windows` 为空 → 拒绝（不猜窗口）。
- 对每个绑定窗口，取 `window.task.verify[]` 中 `kind == criterion.evidence_kind` 的项；多个就按顺序跑，任一跳出且 kind 匹配即用。
- `cpp_build`/`automation` 走 `core.judge` 的真实路径（会关编辑器/编译/跑测试），网关超时 320s；失败原文进 `summary`，截 240 字。
- `apply_judge` 仍是唯一写 `passed` 的入口；`verify` 不落任何模型文本。

#### P3.4 检查

新增 `eval/check_dsh_contract.py`（fixture run JSON + 假 judge，不连编辑器）：

- 未过条款 + 匹配 judge → `passed: true`；
- kind 不匹配、`manual`、模型文本「已完成」→ `passed` 不变；
- 条款绑定窗口缺失 → verify 拒绝且不写；
- `contract_status` 在「自动全过、仍有 manual」时输出「部分完成」；
- 打印 `dsh-contract ok`。

完成标记：`check_dsh_contract.py`、`check_dsh_bridge.py` 双绿；手工在 DSH 里跑「编译工程」→ `verify` → `read_contract`，`cpp_build` 条款从 false 变 true（失败则保持 false）。
feature id：`dsh-skills`、`dsh-contract`。
回滚：删技能与工具入口；`criteria` 数据不变。

---

### 阶段 P4 · 编排与交接（5–7 天）

**目标**：DSH 承担「只读并行研究与任务书」，执行仍回 ue-rag；交接文件两边同格式。

#### P4.1 只读工作流模板

新增 `RAG/ue-rag/dsh/workflows/research.js`、`triage.js`（DSH `workflow` 工具的模板，不是新运行时）：

- `research.js`：入参 `question`；`pipeline`：`search_project`（1 个子代理）→ 代码/文档阅读（2–3 个子代理）→ 汇总 `data/briefs/<ts>-<slug>.md` + `.json`；只读，不调 exec。
- `triage.js`：错误原文 → `search_project(where={"source_type":"ue_error"})` + `documents/排障条目.md` → 输出一行根因与下一步；只读。
- `dsh/README.md` 写清 token 预算：每个 workflow ≤6 个子代理、结果 ≤8KB、禁止把整份文件塞进子代理提示。

#### P4.2 handoff 双向

- `agent/dsh_cli.py handoff-validate` 已具备；P4 增加 `handoff-merge`（把 DSH 子代理产出的 handoff 合并进 `data/handoffs/`，校验通过才写）。
- DSH 技能 `ue-dsh-handoff-skill` 写明：一个窗口一份文件，命名 `<run_id>/<window_id>.json`；`unmet_total/omitted` 口径与 `harness.build_handoff` 一致。

#### P4.3 写锁与冲突

- 新 `eval/check_dsh_lock.py`：两个线程同时 `exec_editor_tool`（mock），断言一成功一 `editor busy`；stale 锁 900s 后可获取。
- DSH 侧规则写进技能：只读可并行；写/编译/PIE 必须串行，收到 `editor busy` 就等待或改走 `run_windows`。

#### P4.4 A/B 量化

新增 `eval/dsh_ab.py`：对三条固定任务（`editor-cube`、`bp-gun-compile`、`cpp-typo`）比较：
- 菜单 2 路径：从 `data/traces`/`costs.jsonl` 取 prompt token 与首次通过；
- DSH 路径：人工跑一次，把 DSH 会话 token 用量与工具调用数填进 `data/harness/dsh_ab.json`（模板由脚本生成，数值人工填或 P5 导入）。
输出 `data/harness/dsh_ab.json`，报告里不宣称「谁更快」，只列数字。

完成标记：`check_dsh_lock.py` 绿；`dsh_ab.json` 三个任务都有两侧数字；`check_upgrade.py` 仍 `upgrade ok`（快照迁移后行为不变）。
feature id：`dsh-orchestration`、`dsh-handoff`。
回滚：删 `dsh/workflows` 与 A/B JSON；不影响现有入口。

---

### 阶段 P5 · 观测与成本（2 天）

**目标**：DSH 侧的花费能被记进同一本账，但不引新运行时。

#### P5.1 导入脚本

新增 `scripts/import_dsh_costs.py`：

- 首选：用 DSH 自带的会话日志导出（设置 → 会话日志导出）产出 JSONL，脚本读取 `usage` 字段；
- 追加到 `data/costs.jsonl`，字段 `caller="dsh"`、`model`、`prompt_tokens`、`completion_tokens`、缓存字段（有则写）、`ts`、`session`（短 id）；
- 只写 token 数，不写 prompt/正文/路径，不写密钥；
- 无 usage 的会话跳过并打印一行，不估算。

#### P5.2 报表

- `eval/report.py` 增加一列或一节：按 caller 分组（understand/planner/plan/dsh），无 `dsh` 数据时整节省略，不填 0。
- 新增 `eval/check_dsh_costs.py`：用 fixture 会话导出 → 断言 rows 追加、caller=dsh、token 为正、没有提示词字段；打印 `dsh-costs ok`。

完成标记：`check_dsh_costs.py` 绿；`report.py` 对 `data/traces` 的既有行为不变（无 dsh 数据时输出与今天一致）。
feature id：`dsh-costs`。
回滚：删导入脚本与 fixture；`costs.jsonl` 只追加不回删（如需回滚，按 ts 过滤）。

---

### 阶段 P6 · 文档与收口（1–2 天）

**目标**：人和下一位代理都能从仓库读懂「DSH 这条入口怎么用、边界在哪」。

1. `WORKFLOW.md` 新增一节「DeepSeek Harness（可选外壳）」（≤60 行）：入口、工具面、策略文件、禁止项、两个检查命令；不重写正文。
2. `RAG/ue-rag/README.md` 增一行入口与 `dsh/README.md` 链接。
3. `documents/harness/overview.md` 增「入口」一行；`context-tiers.md` 增 DSH 的 Tier 说明（AGENTS.md 常驻 + 技能按需 + MCP 检索）。
4. `shortcuts/dsh.bat` 与 `dsh/README.md` 补齐排障：E 盘 ACL 沙箱授权失败时先跑 `diagnose-windows-sandbox-acl` 技能或在 DSH 设置里临时 Full access；不要在 UE 工程里用 Full access 跑写任务。
5. `feature_list.json`：把 `dsh-config`、`dsh-bridge`、`dsh-graph-write-policy`、`dsh-skills`、`dsh-contract`、`dsh-orchestration`、`dsh-handoff`、`dsh-costs` 的 `passed` 按对应检查结果改写（先 pending，检查通过后再 true，evidence 指向检查文件）。
6. `.github/workflows/offline.yml` 追加 `check_dsh_config.py`、`check_dsh_bridge.py`、`check_dsh_contract.py`、`check_dsh_lock.py`、`check_dsh_costs.py`（若 workflows 允许；不允许则只在 WORKFLOW 记录本机命令）。
7. 跑 `python agent/garden.py` 补抬头，确认 `garden_report.json` 无新增 stale。

完成标记：九条原有检查 + 五条新检查全部打印对应 ok；`git status` 只含第 13 节允许的路径；`WORKFLOW.md` 新增节 ≤60 行。
feature id：无新增。
回滚：文档改动可整体 revert（按文件）。

---

## 7. 验收矩阵

工作目录 `RAG/ue-rag`，Python 用 `.venv`。不连编辑器、不调模型为默认；标「真机」的才连。

| # | 命令 | 期望 | 阶段 |
| --- | --- | --- | --- |
| 1 | `python eval/check_routes.py` | `routes ok`（末尾自动跑 upgrade） | P0/P2/P6 |
| 2 | `python eval/check_harness.py` | `harness ok` | P0/P6 |
| 3 | `python eval/check_info_layer.py` | `info ok` | P0/P6 |
| 4 | `python eval/check_architecture.py` | `architecture ok`（新增模块不越界） | P0/P2/P6 |
| 5 | `python eval/check_schema.py` | `schema ok` | P0/P6 |
| 6 | `python eval/check_risk.py` | `risk ok` | P0/P6 |
| 7 | `python eval/check_automation.py` | `automation ok` | P0/P6 |
| 8 | `python eval/check_scene_gate.py` | 对应 ok | P0/P6 |
| 9 | `python eval/check_distill.py` | `distill ok` | P0/P6 |
| 10 | `python eval/check_dsh_config.py` | `dsh-config ok` | P1/P6 |
| 11 | `python eval/check_dsh_bridge.py` | `dsh-bridge ok` | P2/P6 |
| 12 | `python eval/check_dsh_contract.py` | `dsh-contract ok` | P3/P6 |
| 13 | `python eval/check_dsh_lock.py` | `dsh-lock ok` | P4/P6 |
| 14 | `python eval/check_dsh_costs.py` | `dsh-costs ok` | P5/P6 |
| 15 | 真机：DSH 里问「项目里有哪些金币相关资产」 | 调 `mcp__uerag__search_project`，返回带 `source_type/asset_path/class_name` | P1/P6 |
| 16 | 真机：DSH 里请求改 Actor，`session_task.json` 为空 | `write refused` + `policy`，工程文件未变 | P2/P6 |
| 17 | 真机：DSH 里说「已完成」但不 verify | `read_contract` 仍有 false；回复不改变合同 | P3/P6 |
| 18 | 真机：DSH 里跑「编译工程」+ verify | `cpp_build` 条款按真实裁判结果改，失败保持 false | P3/P6 |
| 19 | 真机：两个 DSH 子代理同时写 | 一个成功、一个 `editor busy`，无并发写 | P4/P6 |

---

## 8. token 与成本

| 路径 | 模型调用 | 预算/目标 | 说明 |
| --- | --- | --- | --- |
| 菜单 2/3 固定句式（L0） | 0 | 不变 | 本计划不碰 `plan_windows` / 直接动作 |
| L1 理解器 | 1 | ~1k prompt | 不变 |
| 图内执行 | 每轮 1 | 现有水平 | 不变；不因 DSH 改动 |
| DSH 研究与编排 | 每步 ≥1 | 单任务 research ≤ 25k prompt（P4 记分） | DSH 常驻前缀粗估 8–15k token，靠缓存摊薄；不追求用它跑固定句式 |
| DSH 写/编译 | 每步 ≥1 | 单任务 ≤ 菜单 2 的 2 倍 prompt（P4 A/B 记录） | 只用于交互式单步，不用于批量 |

约束：

- 网关工具 schema 总数 ≤9，单 schema 描述 ≤300 字；不接 830 工具。
- DSH 技能目录：`catalogDescriptionMaxLength: 240`，技能总数 ≤25（19 + DSH 专用 4–6）。
- 自动压缩默认保留，但长会话（>40 轮）先 `/compact` 手动压，避免自动压缩多付一次模型调用。
- 所有对比以 `data/harness/dsh_baseline.json` 为基线；不达标就停在该阶段，不加新功能。

---

## 9. 风险与回滚

| 风险 | 触发迹象 | 缓解 | 回滚 |
| --- | --- | --- | --- |
| 把 830 工具接进 DSH | 工具列表出现大量 `mcp__unreal__*`；请求 token 暴涨 | 网关只暴露 9 工具；`check_dsh_config.py` 断言无全量 server | 删对应 entry |
| DSH 绕过写闸门 | DSH 用 read/write 直接改 `.uasset/.umap` 或 `Source/` | 根 AGENTS.md 明令 + DSH `workspace-write` 沙箱 + 网关唯一写入口；文件级 `.uasset` 二进制不可读也难改 | 收紧文件策略；停止 DSH 写任务 |
| 模型自称完成 | 回复「已完成」但合同 false | `ue-dsh-verify-skill` + `verify` 工具 + AGENTS 三句话；合同只读 | 删技能不解决问题→检查 `apply_judge` 是否被绕开 |
| `session_task.json` 被模型改大 | DSH 用 write 改策略文件 | 该文件在 `data/dsh/`，AGENTS 规定只人写；网关只读；可用文件只读属性 + 检查告警 | 从 `C:\Users\X\.dsh\backups\ue-rag\` 恢复 |
| run JSON 并发写坏 | verify 与 run_windows 同时写 | `data/locks/run-<id>.lock` + 临时文件原子替换 | 从 `data/runs` 备份/归档恢复 |
| 技能重复发现 | 目录出现两份同名技能 | 只配 `.codex/skills`；不配 `.cursor/skills` | 删 custom 根 |
| DSH 压缩多付调用 | 长会话成本上升 | 手动 `/compact`；短任务保持默认；P4 记分 | 设 `compaction-basic.auto: false` |
| plan mode 不阻断工具 | 以为开了 plan 就安全 | 不依赖 plan mode；用沙箱 + 网关锁 | —— |
| profile patch 覆盖整条 config | 改配置后丢字段/启动失败 | patch 只写必要字段；改前备份；`--dump-config` 校验 | 用 P0 备份恢复 |
| E 盘沙箱授权失败 | DSH 报 `SetNamedSecurityInfoW ... grantWrite` | 跑 `diagnose-windows-sandbox-acl` 技能修复；临时 Full access 只用于研究任务，不跑 UE 写 | 切回 workspace-write |
| 隐私：DSH 会话日志上传 | DSH 默认会话日志贡献 | 接入前在 DSH 设置里确认会话日志/反馈开关；不在 DSH 会话里贴密钥 | 关闭贡献/清理会话 |

---

## 10. 里程碑

| 里程碑 | 内容 | 预计 | 门禁 |
| --- | --- | --- | --- |
| M1 | P0 + P1 | 1.5 天 | `dsh-config ok` + DSH 能看到技能与 MCP |
| M2 | P2 | 3–4 天 | `dsh-bridge ok` + 写闸门仍拒绝 |
| M3 | P3 | 3 天 | `dsh-contract ok` + 合同不可被模型改 |
| M4 | P4 | 5–7 天 | `dsh-lock ok` + A/B 有数 |
| M5 | P5 + P6 | 3 天 | `dsh-costs ok` + 全部九条检查绿 |

总计 3–4 周，顺序不可交换：P2 依赖 P1 的 profile 配置；P3 依赖 P2 的 verify；P4 依赖 P3 的技能；P5 依赖 P4 的会话样本。

---

## 11. 完成定义

同时满足才叫「深度绑定完成」：

1. 九条原有离线检查 + 五条新检查全部打印对应 ok；不调真实模型、不连编辑器（真机项单列）。
2. DSH 里 3 个真机验收通过：检索、写拒绝、verify 改合同。
3. DSH 工具列表 ≤9 个、无 830 工具；`session_task.json` 缺失时写操作全拒。
4. 模型正文无法改 `passed`；`manual` 永远 false；`read_contract` 与 `harness` 一致。
5. `data/harness/dsh_baseline.json` 与 `data/harness/dsh_ab.json` 有数；DSH 研究任务 token 不超预算。
6. `WORKFLOW.md` / `README.md` / `AGENTS.md` / `documents/harness/*` 同步；`git status` 仅含允许路径。
7. 现有菜单 1/2/3 行为不变；`Source/`、`Content/`、`.env` 值、LangGraph 边均未改。

---

## 12. 不做

- 不用 DSH 替换 `run_windows`、`judge`、`editor_exec`、`rag_core`。
- 不把 830 工具、全量 schema、`ue_mcp_index.json` 接进 DSH 上下文。
- 不新增第二张业务图、不把循环改成 DSH ReAct、不装 Docker/监控栈。
- 不让 DSH 的 auto-review 或模型审模型替代 `core/judge.py`。
- 不把 `allow_writes` / `save` / `pie` 暴露成 DSH 工具参数。
- 不接 DSH 的自动压缩到 `run_windows`（压缩只影响 DSH 会话，不影响图）。
- 不改 `.env` 的五个 `UE_ALLOW_*`；DSH 路径的安全性由 `session_task.json` 从零开始授权。
- 不做多租户、不做云沙箱、不改模型供应商与 `base_url`。

---

## 13. 批准清单（批准后 pre_authorized 改 true）

允许写入：

- **仓库外**：`C:\Users\X\.dsh\profiles\desktop\cordis.patch.yml`（追加/替换指定条目）；`E:\UEProject\AIWork\AGENTS.md`
- **AIAgentProject**：`AGENTS.md`、`WORKFLOW.md`（只增节）、`README.md`（只增行）、`.codex/skills/ue-dsh-*/`、`.cursor/skills/ue-dsh-*/`、`.codex/hooks/refuse_graph_writes.py`、`.cursor/hooks/refuse_graph_writes.py`、`documents/harness/overview.md`、`documents/harness/context-tiers.md`
- **RAG/ue-rag**：`dsh/`（新建）、`server/dsh_bridge.py`、`agent/dsh_cli.py`、`agent/snapshot.py`、`agent/run_windows.py`（只改快照 import）、`core/graph_write_policy.py`、`core/editor_exec.py`（仅在必要时接 policy，不改拒绝链语义）、`pipelines/export_skill_cards.py`（只加 DSH 技能注册）、`data/skill_cards.json`（重新生成）、`eval/check_dsh_*.py`、`eval/dsh_baseline.py`、`eval/dsh_ab.py`、`eval/check_routes.py`（hook 断言）、`eval/check_upgrade.py`（快照引用）、`eval/check_architecture.py`（仅当新模块需要边界断言）、`eval/report.py`（DSH 节）、`scripts/import_dsh_costs.py`、`shortcuts/dsh.bat`、`shortcuts/dsh_preflight.py`、`data/harness/feature_list.json`（只追加）、`data/harness/dsh_baseline.json`、`data/dsh/`（运行态 + `session_task.example.json`）、`.gitignore`、`.github/workflows/offline.yml`
- **禁止写入**：`Source/`、`Content/`、`Saved/`、`Binaries/`、`Intermediate/`、`.env` 的值、`agent/graph.py` 的边、`core/judge.py` 的判定语义、`data/skills/learned_skills.json` 的已有条目

执行纪律：

- 交给 Cursor：按 `Plan/DeepSeekHarness绑定执行/00-…` 到 `05-…` 逐卡执行；每卡开工前把该卡 `pre_authorized` 改 `true`（或在对话里明确批准）。
- 每阶段结束先跑验收，通过再写 `feature_list.json` 的 `passed`。
- 失败最多修一次；第二次失败写「执行记录」并停。
- 每阶段完成后在文末「执行记录」追加：日期、阶段、命令、结果、遗留。

---

## 14. 执行记录

（实施时追加，格式：`YYYY-MM-DD Pn · 命令 → 结果 · 遗留`）

- 2026-10-23 · 计划编写完成（未执行）。前置分析：`DeepSeekHarness融合分析.md`、`.analysis/ue-system-analysis.md`、`.analysis/dsh-capabilities.md`；`.cursor` 评估见第 4 节。
- 2026-10-23 P0+P1 · 基线九条检查绿（`check_routes` 首次因新技能未注册卡片失败，补 `REGISTRY` 后 `routes ok`）；`dsh_baseline` 995/1022/8008/106；`check_dsh_config ok (4 entries, 20 skills)`；`preflight ok`；`feature_list` 新增 `dsh-config` 已 passed。遗留：profile patch 与 DSH 真机三步待人在 DSH 里完成。
- 2026-10-23 P2 · 图写判定单源化（`core/graph_write_policy.py`，两个 hook 与网关共用）；快照拆到 `agent/snapshot.py`；`agent/dsh_cli.py` + `server/dsh_bridge.py`（9 工具，stdio 实拉通过）；`check_dsh_bridge ok`、`architecture ok`、`routes ok`、`upgrade ok`、`harness ok`、`dsh-config ok (3 entries, 20 skills)`；`dsh-bridge`、`dsh-graph-write-policy` 已 passed。遗留：最终 3 条 patch 替换与 DSH 真机 9 工具/写拒绝验证待人在 DSH 里完成。
- 2026-10-23 P3 · 三张技能 `ue-dsh-route/verify/handoff`（`.codex` 与 `.cursor`）；`dsh_cli` verify 改为空窗口不猜、第一个 kind 匹配即用、`--all` 不中断、`--dry-run` 不写、拒绝带 policy；`check_dsh_contract ok`、`check_dsh_bridge ok`、`check_dsh_config ok (3 entries, 23 skills)`、`harness ok`；`dsh-skills`、`dsh-contract` 已 passed。遗留：DSH 里确认三个技能，并用真机 `read_contract` → `verify` 对一下合同。
- 2026-10-23 P4（补记，详见执行卡 04）· `dsh/workflows/research.js`（5 子代理）+ `triage.js`（2 子代理）+ `workflows/README.md`；`handoff-validate` 按 `build_handoff` 口径重写、新增 `handoff-merge`；`dsh_cli` 写锁 `_lock_stale`/`_pid_alive`；`eval/check_dsh_lock.py`、`eval/dsh_ab.py`、`data/harness/dsh_ab.json`；`dsh-lock ok`、`dsh-bridge ok`、`upgrade ok`、`routes ok`、`dsh-config ok (3 entries, 23 skills)`；110 份现存交接全部通过校验；`dsh-orchestration`、`dsh-handoff` 已 passed。人工 workflow 真跑通：`research.js` → `data/briefs/20261023-175218-testpawn-wasd-movement.md`，`triage.js` 命中 `error C2039 PatrolSpeed`。遗留：`dsh_ab.json` 的 DSH 侧三项数字仍为 null，需人按三条固定任务各跑一次填。
- 2026-10-23 P5+P6（详见执行卡 05）· 新 `scripts/import_dsh_costs.py`（读 `data/dsh/exports/*.jsonl`，只抄 usage 成 `caller="dsh"` 行，无 usage 跳过不估算，不抄正文/路径/密钥）、`eval/check_dsh_costs.py`（8 组断言，临时目录）、fixture 3 行；`eval/report.py` 加「DSH 用量」节（无 dsh 行整节省略，既有函数签名与行为未改）；`.github/workflows/offline.yml` 加 `check_dsh_contract`/`check_dsh_lock`/`check_dsh_costs`；`.gitignore` 给 fixture 开例外；`feature_list` 追加 `dsh-costs`；`WORKFLOW.md` 新增 DSH 一节（43 行 ≤60）、`overview.md`/`context-tiers.md`/`README.md`/`dsh/README.md`/`dsh.bat` 同步。全量验收：九条原有检查 + 五条 DSH 检查全部 `ok`（`routes`/`harness`/`info`/`architecture`/`schema`/`risk`/`automation`/`scene gate`/`distill`/`dsh-config`/`dsh-bridge`/`dsh-contract`/`dsh-lock`/`dsh-costs`）；`dsh-baseline 48/995 18/1022 133/8008 2/106`；`garden scanned=8 patched=0 stale=0`；183 份交接 `ok=183 bad=0`。八条 DSH feature 全部 `done/passed`。遗留（人工门）：真实 DSH 会话导出尚未导入（账上仍无 `caller=dsh` 行）、DSH 真机三步未做、会话日志上传开关未确认。

