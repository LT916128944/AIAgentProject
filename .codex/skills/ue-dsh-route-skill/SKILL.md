---
name: ue-dsh-route-skill
description: DSH 里要动编辑器、编译或跑 Automation 时用。先扫工具卡片，再取一个 schema，再执行。保存和 PIE 只读 session_task.json。
---

# DSH 编辑器路由

触发：要动编辑器、编译、跑 Automation。

1. 顺序固定：`scan_editor_tools`（卡片，无 schema）→ `get_editor_tool_schema`（只取一个）→ `exec_editor_tool`。
2. 同一编辑器任务最多 8 次调用。扫描换一种说法再扫一次，仍不相关就停，只报「未知工具」。
3. 失败把原文交给人，最多重试一次。
4. 保存、PIE、写前缀只看 `RAG/ue-rag/data/dsh/session_task.json`。不得要求人改这个文件，参数里不写 `allow_writes`、`save`、`pie`。
5. 纯问答、只改 C++（`edit_cpp` / `run_task`）、只查资产（`search_project`）不扫描编辑器工具。

禁止：拉全量 schema、`list_toolsets`、把 830 个工具装进上下文、读 `ue_mcp_index.json`。
