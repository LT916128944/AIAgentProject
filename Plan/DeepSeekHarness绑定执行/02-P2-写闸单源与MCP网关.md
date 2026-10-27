---
id: dsh-exec-02
priority: 80
pre_authorized: true
ask_user: false
stop_to_confirm: false
session: 1
depends_on: [dsh-exec-01]
last_updated: 2026-10-23
status: done
owner: 侑
---

# 02 · P2 写闸门单源化 + 薄 MCP 网关

核心目标：让 DSH 只能「调用」执行器与裁判，不能「改写」它们。工具总数 ≤9，830 工具永不进 DSH。

## 0. 允许写入

`RAG/ue-rag/`：`core/graph_write_policy.py`（新）、`core/editor_exec.py`（只在必要时接入策略，不改拒绝链语义）、`agent/snapshot.py`（新）、`agent/run_windows.py`（只改快照 import）、`agent/dsh_cli.py`（新）、`server/dsh_bridge.py`（新）、`eval/check_dsh_bridge.py`（新）、`eval/check_routes.py`（hook 断言）、`eval/check_upgrade.py`（快照引用）、`data/harness/feature_list.json`（只追加）、`data/dsh/session_task.example.json`（新）、`data/dsh/`（运行态）、`data/spill/`、`data/locks/`、`.gitignore`、`dsh/cordis.patch.yml`（改成最终态）、`dsh/README.md`（补工具面与策略说明）
`AIAgentProject/`：`.cursor/hooks/refuse_graph_writes.py`、`.codex/hooks/refuse_graph_writes.py`（改薄包装）、`AGENTS.md`（只追加策略三句，若 P1 未加）

禁止：`Source/`、`Content/`、`.env`、`agent/graph.py`、`core/judge.py` 语义、`data/skills/*.json`、新增 `check_architecture.py` 白名单。

## 1. 开工只读

1. 本卡；`00-给Cursor的开工说明.md`
2. `AIAgentProject/.cursor/hooks/refuse_graph_writes.py` 全文（119 行，要迁走）
3. `RAG/ue-rag/core/editor_exec.py` 的 `exec_tool` 与 `_refused`
4. `RAG/ue-rag/agent/harness.py` 的 `apply_judge`、`contract_status`、`policy_snapshot`
5. `RAG/ue-rag/agent/run_windows.py` 第 540–650 行（快照函数）
6. `RAG/ue-rag/core/judge.py` 的 `judge(spec, task)` 签名
7. `RAG/ue-rag/server/server.py`（MCP server 写法，`MCPServer("...")` + `@mcp.tool()`）

## 2. 步骤 1 · 写闸判定单源化

1. 新建 `core/graph_write_policy.py`：把 `BP_WRITES`、`_ENV`、`env_flag`、`blueprint_allowed`、`domain_allows`、`_flag`、`_is_read`、`_domain_key`、`decide` 从 `.cursor/hooks/refuse_graph_writes.py` **原样迁入**（行为不变，包括 `UE_ALLOW_*` 读 `.env`、读不到即 false）。
2. `.cursor/hooks/refuse_graph_writes.py` 改为薄包装：
   - 用 `Path(__file__).resolve()` 推出 `E:\UEProject\AIWork\RAG\ue-rag`，`sys.path.insert` 后 `from core.graph_write_policy import decide, blueprint_allowed, domain_allows`；
   - 保留原文的 stdin JSON 解析与 stdout `{"permission": ...}` 输出、`main()` 返回 0；
   - 自测（必须跑）：
     `'{"command":"python scripts/call_mcp_tool.py --tool write_graph_dsl --toolset X"}' | python .cursor/hooks/refuse_graph_writes.py` → 输出 `deny`（`.env` 里 `UE_ALLOW_BLUEPRINT=true` 时换成 `get` 前缀工具验证 allow；另用临时环境读不到 `.env` 的场景验证 fail-closed 由 `check_routes.py` 覆盖）。
3. `.codex/hooks/refuse_graph_writes.py` 同样改薄包装（路径 parents 不同，计算到 ue-rag 根）。
4. `eval/check_routes.py` 的 hook 断言改为 `from core.graph_write_policy import decide`，保留原断言；新增一条：找不到 `.env` 时 `blueprint_allowed()` 为 false。

## 3. 步骤 2 · 快照独立

1. 新建 `agent/snapshot.py`：从 `agent/run_windows.py` 迁入 `_snapshot_source`、`_restore_source` 及其私有依赖，公开为 `snapshot_source(run_id, window_id)`、`restore_source(run_id, window_id, snap)`；路径与字节语义不变。
2. `run_windows.py` 改为 `from agent.snapshot import ...`，不复制逻辑；`check_upgrade.py` 的快照用例保持通过。

## 4. 步骤 3 · 证据 CLI `agent/dsh_cli.py`

只被 MCP 网关以子进程调用，也可人手跑；每个子命令输出单行 JSON，不打印密钥/长正文。子命令：

| 子命令 | 行为 | 约束 |
| --- | --- | --- |
| `resume-line` | 输出 `resume_context.build_line(...)` | 只读 |
| `contract-status --run <id>` | 读 `data/runs/**/<id>.json` 的 `criteria`，调 `harness.contract_status` | 只读 |
| `verify --run <id> --criterion <cid> [--dry-run]` | 由 `criterion.evidence_kind` + `criterion.windows` 取窗口 `window.task.verify[]` 匹配 kind 的 spec，调 `core.judge.judge(spec, task)`，再 `harness.apply_judge`；原子写回 run JSON | `windows` 为空/无匹配 spec → 失败不猜；`manual` 永不 true；`--dry-run` 只算不写 |
| `handoff --run <id> [--window <wid>]` | 读 `data/handoffs/<run>/<wid>.json` | 只读 |
| `handoff-validate --file <path>` | 校验 `did/evidence/unmet/unmet_total/unmet_omitted/next/artifacts`；`unmet`≤8；含 `read_source` 正文则失败 | 只读 |
| `window-state snapshot\|restore --run <id> --window <wid>` | 调 `agent.snapshot` | 不保存关卡、不改 `.env` |

实现要求：
- run JSON 写入用「读锁 `data/locks/run-<id>.lock` → 临时文件 → `os.replace`」；
- `verify` 只改 `criteria` 字段与 `updated_at`，其余字段原样保留；失败不写；
- 所有输出带 `{ok, ...}`；异常转 `{ok:false, error}` 且退出码 1；
- 路径参数只接受 `data/` 下的 run id 形如 `YYYYMMDD-HHMMSS` 或 `run_id`，不接受任意路径。

## 5. 步骤 4 · MCP 网关 `server/dsh_bridge.py`

边界：**只 import `config` 与 `core`**；agent 层一律 `subprocess` 调 `agent/dsh_cli.py`（保持 `check_architecture.py` 依赖方向，不新增白名单）。工具 9 个：

1. `search_project(query, top_k=5, where=None)` → `core.rag_core.retrieve`
2. `scan_editor_tools(query, top_k=8)` → 复用 `scripts/scan_mcp_tools.py --query --top-k`（subprocess，stdout 截断；只返回卡片，无 `input_schema`）
3. `get_editor_tool_schema(full_name)` → 同上 `--schema`
4. `exec_editor_tool(toolset, tool, args)` → 读 `data/dsh/session_task.json` 组 task，调 `core.editor_exec.exec_tool`；`risk.tier` 为 mid/high 或 save/PIE/写前缀时取 `data/locks/editor.lock`（stale 900s）
5. `read_contract(run_id)` → CLI `contract-status`
6. `verify(run_id, criterion_id)` → CLI `verify`
7. `read_handoff(run_id, window_id=None)` → CLI `handoff`
8. `window_state(action, run_id, window_id)` → CLI `window-state`，`action` 只允许 `snapshot|restore`
9. `resume_context()` → CLI `resume-line`

网关硬规则：
- 策略只从 `data/dsh/session_task.json` 读；文件缺失 → 所有写操作返回 `{ok:false, error:"write refused", policy:{...}}`；
- 工具 schema 里**没有** `allow_writes/save/pie/allow_blueprint_ref` 参数（写死在 server 端）；
- 大输出：原文写 `data/spill/dsh-<ts>-<tool>.json`，内联 ≤1KB（`{ok, summary, artifacts:[path], policy}`）；
- 不调 LLM、不出网（embedding 本地）；
- 新增 `data/dsh/session_task.example.json`（可提交）：
  `{"allow_writes": [], "save": false, "pie": false, "allow_blueprint_ref": "", "note": "复制为 session_task.json 后由人按任务改"}`

## 6. 步骤 5 · 检查 `eval/check_dsh_bridge.py`

mock 模式（PowerShell 先 `$env:UE_EXEC_MODE="mock"`，跑完恢复），不连编辑器、不调模型：

- `scan_editor_tools` 返回卡片且无 `input_schema` 字段；
- `session_task.json` 缺失/空 → `exec_editor_tool set_actor_transform` 返回 `write refused` 且带 `policy`；
- `query_actor` 在 mock 下允许；
- `verify`：fixture run JSON + 假 judge → kind 不匹配不改、匹配且 ok 改 true、`manual` 永远 false、模型文本不改；
- `read_contract` 与 `harness.contract_status` 一致；
- 并发两次写 → 第二次 `editor busy`；
- 网关 tool schema 无 `allow_writes/save/pie`；
- 打印 `dsh-bridge ok`。

## 7. 步骤 6 · 切最终 patch

把 `dsh/cordis.patch.yml` 的两条 MCP 替换为一条：

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

`dsh/README.md` 更新：最终工具面 9 个、`session_task.json` 用法、写锁、spill 路径、回滚。

`feature_list.json` 追加两条（只追加）：
`dsh-bridge`（evidence `eval/check_dsh_bridge.py`）、`dsh-graph-write-policy`（evidence `eval/check_routes.py`）。

## 8. 验收

```powershell
$env:UE_EXEC_MODE="mock"
python eval/check_dsh_bridge.py    # dsh-bridge ok
Remove-Item Env:UE_EXEC_MODE
python eval/check_architecture.py  # architecture ok
python eval/check_routes.py        # routes ok（含 hook 断言；约 2–3 分钟）
python eval/check_upgrade.py       # upgrade ok
```

hook 自测（Cursor 侧行为不变）：

```powershell
'{"command":"python scripts/call_mcp_tool.py --tool write_graph_dsl --toolset x"}' | & 'E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe' '.cursor\hooks\refuse_graph_writes.py'
# 期望：{"permission":"deny",...}
```

## 9. 人工门

1. 用最终 patch 替换 profile 里的两条 MCP，重启 DSH；
2. 确认工具列表恰好 9 个 `mcp__uerag__*`，无 `mcp__uec__*`、无 830 工具；
3. DSH 里请求改 Actor（`data/dsh/session_task.json` 为空）→ 应被拒且带 `policy`。

## 10. 回滚

- profile 恢复 P1 备份；
- 代码新增文件可保留（不影响现有入口）；若要彻底回滚：删 `core/graph_write_policy.py` 并恢复两个 hook 原文件（git diff 可见）、删 `agent/snapshot.py` 并恢复 `run_windows.py` 的快照函数、删 `agent/dsh_cli.py`、`server/dsh_bridge.py`、`eval/check_dsh_bridge.py`；
- `feature_list.json` 删本卡追加的两条。

## 11. 执行记录

### 2026-10-23 · P2（DSH 会话执行）

- 新增：`core/graph_write_policy.py`、`agent/snapshot.py`、`agent/dsh_cli.py`、`server/dsh_bridge.py`、`eval/check_dsh_bridge.py`、`data/dsh/session_task.example.json`。
- 修改：两个 hook 改薄包装（判定统一走 core）、`agent/run_windows.py`（快照 import，行为不变）、`eval/check_routes.py`（新增「hook.decide is core.decide」断言）、`eval/check_upgrade.py`（快照 monkeypatch 指到 `agent.snapshot`）、`dsh/cordis.patch.yml`（最终 3 条：网关 + 技能目录 + 目录长度）、`dsh/README.md`、`shortcuts/dsh_preflight.py`（兼容 `mcp-uerag`）、`.gitignore`（`data/locks/`、`data/spill/`）、`feature_list.json`（`dsh-bridge`、`dsh-graph-write-policy` 已按检查改 passed）。
- 命令 → 结果：
  - `python eval/check_dsh_bridge.py` → `dsh-bridge ok`（9 工具 schema、无 `input_schema` 卡片、空策略拒绝写、verify 只由裁判改 passed、manual 不动、dry-run 不写、写锁 busy、坏交接被拒）。
  - `python eval/check_architecture.py` → `architecture ok`（server 只 import config/core，agent 层走子进程 CLI，未加白名单）。
  - `python eval/check_routes.py` → `routes ok`（含两条单源断言）；`check_upgrade.py` → `upgrade ok`；`check_harness.py` → `harness ok`。
  - `python eval/check_dsh_config.py` → `dsh-config ok (3 entries, 20 skills)`。
  - MCP stdio 实拉 `server/dsh_bridge.py` → `tools=9`（search_project / scan_editor_tools / get_editor_tool_schema / exec_editor_tool / read_contract / verify / read_handoff / window_state / resume_context）。
  - `check_routes.py` 首次运行曾因快照 monkeypatch 位置失效失败；改 `check_upgrade.py` 指向 `agent.snapshot` 后通过。
  - mock 测试需 `INJECT_ERRORS=false`（`_mock_tool` 默认 10% 注入），已写进检查脚本。
- 需要人做（真机）：
  1. 用最终 `dsh/cordis.patch.yml`（3 条）替换 profile 里 P1 的 4 条，重启 DSH；
  2. 确认工具列表恰好 9 个 `mcp__uerag__*`、无 `mcp__uec__*`；
  3. 保持 `data/dsh/session_task.json` 为空策略，请求改 Actor → 应 `write refused` + `policy`；
  4. 已把本卡要求的两条提醒（hook 自测、空策略）保留在 §8/§9。

### 2026-10-23 · 补记（DSH 会话，P3 复核）

- 人工门 1 已执行：profile 里此前从未落地过 P1 的 4 条，本次直接追加最终 3 条（`mcp-uerag` / `skill-filesystem` / `tool-skill`），备份 `C:\Users\X\.dsh\backups\ue-rag\cordis.patch.20261023-164129.yml`。
- 验证：按 profile 里的 command/args/cwd stdio 实拉 `server/dsh_bridge.py` → 工具数 9；`shortcuts/dsh_preflight.py` → `preflight ok`。
- 仍待：重启 DSH 后做人工门 2/3（确认 9 个 `mcp__uerag__*`、空策略改 Actor 应 `write refused` + `policy`）。

### 2026-10-23 · 补记修正（DSH 会话）

- patch 方言修正：`mcp-uerag` 必须在 `- insert:` 里（裸行被跳过）；`skill-filesystem` / `tool-skill` 按 id 覆盖。新备份 `C:\Users\X\.dsh\backups\ue-rag\cordis.patch.20261023-165210.yml`。
- 人工门 2 已验证：DSH 会话已加载 9 个 `mcp__uerag__*`，`resume_context` / `read_contract` / `verify` 均可调用。
- 人工门 3 用空策略请求写 Actor 会在编辑器连接前被网关拒绝（`session_task.json` 缺失 fail-closed），真机写场景留待有编辑器时再验。
