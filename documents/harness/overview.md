---
last_updated: 2026-10-29
status: active
owner: 侑
---

# Harness 进程概览（当前事实，不是目标架构）

## 当前入口

`start.bat` 三条入口共用同一份检索库和同一个编辑器执行器：

- `chat.py`：普通对话，retrieve 后回答
- `run_multi.py`：单循环，与菜单 3 同一条 `plan_windows` 链，每步走同一张 LangGraph
- `run_windows.py`：多窗口，规划者 / 任务书 / 快路径，每扇一次 `run_task`（只读条目可并行过编辑器闸门），配 `board.py` 面板、`brief.py` 任务书 + 人审门、`ledger.py` 台账

Cursor 对话仍按根目录 `AGENTS.md` 说话。`UE_EDITOR_BACKEND=official` 时走 `scan_mcp_tools.py` + `call_mcp_tool.py`，汇合在 `core/editor_exec.py`（8000）。`monolith` 时走 `backends/monolith.py` 的 discover / describe / exec（9316）。`backends/select.py` 一次运行只锁一个，选错入口拒绝，不回退另一边。本仓库 `.env.example` 正在用的是 `monolith`。

- DeepSeek Harness（可选外壳）：DSH 打开工作区 `E:\UEProject\AIWork`，仍是 9 个 `mcp__uerag__*`。扫描、取 schema、执行在 `server/dsh_bridge.py` 转到已锁定的后端。配置模板 `RAG/ue-rag/dsh/cordis.patch.yml`，说明 `RAG/ue-rag/dsh/README.md`。写策略只读 `RAG/ue-rag/data/dsh/session_task.json`，完成证据只认 `read_contract` + `verify`。

## 当前架构事实

- **一张 LangGraph**。节点顺序 `retrieve → plan → execute → build → test → observe → fix`，边由 `RAG/ue-rag/eval/check_routes.py` 固定，边没有为任何新功能增删过。
- **一次运行一个编辑器出口**。`official` 是 `core/editor_exec.py`。`monolith` 是 `backends/monolith.py`，场景/资产/材质原语经 `core/monolith_bridge.py` 翻译成对应 action，映射外带原因拒绝。裁判在 `core/judge.py` 按已锁定的后端分叉。协议仍是 MCP HTTP。
- **网关 `server/gateway.py` 在 127.0.0.1:8010**，四个路由：`/api/health`、`/api/chat`（一次补全）、`/api/structured`（理解器）、`/api/tool-call`（只转发契约里的六个工具）。网关不拆窗口、不跑 LangGraph。
- **思考在 L1，归地和裁判在代码**：句子 → 动作是 L0 手写规则 / 已学句式 → L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径，asset 槽位走索引）→ L3 学习回写。只读诊断句由 `agent/diagnose.py` 归到唯一的 Automation 过滤器，收成已有的 `run_automation_test`，不扩动作表。`backend=monolith` 时蓝图类动作对同一目标最多修 3 次。`build` / `test` 的裁判 `core/judge.py` 不带 LLM，成功与否不由模型宣布。零窗口范围补窗失败且 `UE_SOLVER_ENABLED=true` 时补一扇受控求解扇 `solve`（`agent/solver.py`，预算闸门 + automation 收口，缺省 false；一个任务书最多一扇，全部未覆盖交付合并进去；省略过滤器时从真测试清单补选一次，补扇对不上未覆盖交付就不补，台账 `done` 要有改动证据）；`apply_spec` 写完源码强制编译（跨进程编译锁），错误原文进下一轮，编译成败都回连编辑器（`agent/build_error.py`）；范围有窗但交付项没被窗口接住时做覆盖对账，求解关着记 `partial_gap`、交付验收判 False（`agent/brief.py`）。
- **检索不进规划者**：`retrieve` 只为路由与句子服务；专有名词唯一命中即停，否则 BM25 + bge RRF 合并，热路径无重排。
- **决策落盘**：每次模型调用记 `data/costs.jsonl`；事件流 `context_slice` 记 `user / judge_fail / tool_kept / tool_dropped` 四个计数。

## 进度状态

做到哪一步，看 `RAG/ue-rag/data/harness/progress.json`；某功能真完成没有，看同目录 `feature_list.json`。两份都是机器可读状态，会话开工从它们重建上下文，不再从长文找进度。

本概览过期时以 `feature_list.json` 为准。

## 2026-10-29 新增：确定性玩法链（零模型）

FPS HUD 整句（`打开编辑器，在 FPS 关卡加一个 UMG 到视口…`）现在由 `agent/fps_hud.py: hud_windows` 直接拆成五扇固定因果链，`plan_windows` 标 `source=hud-rules`，`run_windows._needs_brief` 对其跳过任务书人审门；控件树由 `build_fps_hud` 用 monolith `ui.*` 原语确定性落地（不再 discover 猜 action），`check_fps_hud` 只读收口六个合同控件名 + 父类 `FpsHUDWidget` + C++ 挂视口入口。实测 run `20261029-123907`：5 扇全绿、退出码 0、`data/costs.jsonl` 零新增（0 token）。

这是「复杂玩法需求低 token」的第一个完整样板：能写死因果链的句子不叫模型，模型只处理真正有裁量的部分。残差（控件事件图绑定、命名槽）见 `Plan/20261029-1240-FPSHUD与UMG桥后续升级.md`。