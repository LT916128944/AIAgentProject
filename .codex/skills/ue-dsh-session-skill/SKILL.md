---
name: ue-dsh-session-skill
description: DSH 会话开工与 UE 操作路由。先 resume_context；编辑器先扫卡片再取单项 schema；写策略只读 data/dsh/session_task.json；任何完成都要 read_contract + verify。
---

# DSH 会话（第二入口）

本技能只补 DSH 与 Cursor/Codex 的差异；九条铁律与域技能仍以根 `AGENTS.md`、`.codex/skills` 为准。

## 1. 开工

1. 调 `mcp__uerag__resume_context`；该工具还不存在时，读 `data/harness/progress.json` 与 `feature_list.json` 的第一条 `pending`。
2. 不读 `WORKFLOW.md` 全文、不读蒸馏目录、不读 `ue_mcp_index.json`。

## 2. 路由

- 编辑器：`scan_editor_tools`（卡片，无 schema）→ `get_editor_tool_schema`（只取一个）→ `exec_editor_tool`。
- 同一编辑器任务 ≤8 次调用；扫两轮仍不相关就停，只说「未知工具」，不回退全量 schema。
- 资产语义查询：`search_project`。排障：`search_project(where={"source_type":"ue_error"})` 或 `doc_kind=debug`，`top_k=1`。
- 改 C++：走菜单 2/3 的 `run_task`，不要在 DSH 里直接写 `Source/`。

## 3. 写策略

- `allow_writes` / `save` / `pie` / `allow_blueprint_ref` 只从 `RAG/ue-rag/data/dsh/session_task.json` 读，该文件只由人写。
- 工具参数里没有这些字段；不要请求模型自己放宽权限。
- 收到 `write refused` 时把 `policy` 原样交人，不换工具重试。

## 4. 完成

- 先 `read_contract(run_id)`；再对未过且非 `manual` 的条款调 `verify(run_id, criterion_id)`。
- 引用返回的 `{ok, kind, summary}`；`manual` 永远 false；连续两条 verify 失败就停，交人。
- 模型正文里的「已完成」不是证据。
