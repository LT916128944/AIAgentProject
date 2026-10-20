---
last_updated: 2026-10-20
status: active
owner: 侑
---

# Harness 进程概览（当前事实，不是目标架构）

## 当前入口

`start.bat` 三条入口共用同一份检索库和同一个编辑器执行器：

- `chat.py`：普通对话，retrieve 后回答
- `run_multi.py`：单循环，与菜单 3 同一条 `plan_windows` 链，每步走同一张 LangGraph
- `run_windows.py`：多窗口，规划者 / 任务书 / 快路径，每扇一次 `run_task`（只读条目可并行过编辑器闸门），配 `board.py` 面板、`brief.py` 任务书 + 人审门、`ledger.py` 台账

Cursor 对话仍按根目录 `AGENTS.md` 说话，编辑器操作走 `scan_mcp_tools.py` + `call_mcp_tool.py`，与闭环汇合在同一个 `editor_exec.py`。

## 当前架构事实

- **一张 LangGraph**。节点顺序 `retrieve → plan → execute → build → test → observe → fix`，边由 `RAG/ue-rag/eval/check_routes.py` 固定，边没有为任何新功能增删过。
- **`RAG/ue-rag/core/editor_exec.py` 是唯一编辑器出口**。Cursor CLI 和 LangGraph 都走它，协议仍是 `ue_mcp_client.call_ue_tool`。
- **网关 `server/gateway.py` 在 127.0.0.1:8010**，四个路由：`/api/health`、`/api/chat`（一次补全）、`/api/structured`（理解器）、`/api/tool-call`（只转发契约里的六个工具）。网关不拆窗口、不跑 LangGraph。
- **思考在 L1，归地和裁判在代码**：句子 → 动作是 L0 手写规则 / 已学句式 → L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径，asset 槽位走索引）→ L3 学习回写。`build` / `test` 的裁判 `core/judge.py` 不带 LLM，成功与否不由模型宣布。
- **检索不进规划者**：`retrieve` 只为路由与句子服务；专有名词唯一命中即停，否则 BM25 + bge RRF 合并，热路径无重排。
- **决策落盘**：每次模型调用记 `data/costs.jsonl`；事件流 `context_slice` 记 `user / judge_fail / tool_kept / tool_dropped` 四个计数。

## 进度状态

做到哪一步，看 `RAG/ue-rag/data/harness/progress.json`；某功能真完成没有，看同目录 `feature_list.json`。两份都是机器可读状态，会话开工从它们重建上下文，不再从长文找进度。

本概览过期时以 `feature_list.json` 为准。