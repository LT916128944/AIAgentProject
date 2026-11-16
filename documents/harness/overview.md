---
last_updated: 2026-11-11
status: active
owner: 侑
---

# Harness 进程概览（当前事实，不是目标架构）

## 当前入口

入口页 `RAG/ue-rag/server/static/entry.html`（由同仓库 `server/entry.py` 在 127.0.0.1:8011 提供；双击 `AIAgentProject\start.bat` 一步起）上的三种模式共用同一份检索库和同一个编辑器执行器：

- `chat.py`：普通对话，retrieve 后回答
- `run_multi.py`：单循环，与多窗口同一条 `plan_windows` 链，每步走同一张 LangGraph。有活心跳时与多窗口一样退出码 2，`--force` 才继续
- `run_windows.py`：多窗口。整句才命中规则或 HUD 链；goals 盖不住时，求解开着进求解扇，关着停在缺口，不再先叫规划者把残句领走。每扇一次 `run_task`（只读条目可并行过编辑器闸门），配 `board.py` 面板、`brief.py` 任务书 + 人审门、`ledger.py` 台账。重定向输入先改名为 `.consumed`；有活心跳时退出码 2，`--force` 才继续并写下 `forced_by`。跑完的整句另记 `ok` / `partial` / `failed`（退出码 0 / 3 / 1）；未签 `manual` 不是 `ok`。退出码 2 仍只表示有活心跳
- 「续接最近」是 `run_windows.py --resume latest`

`RAG/ue-rag/server/entry.py` 是浏览器侧入口（`python server/entry.py --port 8011` 一步拉起：服务会自己打开浏览器，`--no-open` 关掉；端口上已有实例时不抢端口、直接开页面。127.0.0.1:8011）：目录侧边栏 + 拖拽附件 + 装配预览 + 运行面板 + 决策面板。读 `data/runs/*.json` 与 `data/events/*.jsonl`，装配走 `server/prompt_builder.py` 的纯函数。写只有三个文件：`data/ui/req/<ts>.txt`（`POST /api/run` → 对应模式的 agent 子进程，多窗口带 `--plain --force`）、`data/ui/inbox/<run_id>.jsonl`（`POST /api/answer` 一行裁决）、`data/ui/logs/`。裁决通道在 `agent/ui_ask.py`：`question` 事件发在拿答案之前，收件箱按 `run_id`+`question_id` 双键「领取即改名」，等不到答案就按停止收口（不自动 skip）。它不 import `agent`，不改 `run_windows` 的一进程一 run 假设。

Cursor 对话仍按根目录 `AGENTS.md` 说话。`UE_EDITOR_BACKEND=official` 时走 `scan_mcp_tools.py` + `call_mcp_tool.py`，汇合在 `core/editor_exec.py`（8000）。`monolith` 时走 `backends/monolith.py` 的 discover / describe / exec（9316）。`backends/select.py` 一次运行只锁一个，选错入口拒绝，不回退另一边。本仓库 `.env.example` 正在用的是 `monolith`。

- DeepSeek Harness（可选外壳）：DSH 打开工作区 `E:\UEProject\AIWork`，仍是 9 个 `mcp__uerag__*`。扫描、取 schema、执行在 `server/dsh_bridge.py` 转到已锁定的后端。配置模板 `RAG/ue-rag/dsh/cordis.patch.yml`，说明 `RAG/ue-rag/dsh/README.md`。写策略只读 `RAG/ue-rag/data/dsh/session_task.json`，完成证据只认 `read_contract` + `verify`。

## 当前架构事实

- **一张 LangGraph**。节点顺序 `retrieve → plan → execute → build → test → observe → fix`，边由 `RAG/ue-rag/eval/check_routes.py` 固定，边没有为任何新功能增删过。
- **一次运行一个编辑器出口**。`official` 是 `core/editor_exec.py`。`monolith` 是 `backends/monolith.py`，场景/资产/材质原语经 `core/monolith_bridge.py` 翻译成对应 action，映射外带原因拒绝。`judge_pickup` 和依赖 `set_properties` 的 `set_pickable` 在拆句转入 unresolved，不进执行窗口。裁判在 `core/judge.py` 按已锁定的后端分叉。协议仍是 MCP HTTP。
- **网关 `server/gateway.py` 在 127.0.0.1:8010**，四个路由：`/api/health`、`/api/chat`（一次补全）、`/api/structured`（理解器）、`/api/tool-call`（只转发契约里的六个工具）。`/api/health` 在 `ok`、`model`、`mcp_url`、`mcp_up` 之外还有 `backend`、`index_present`、`circuit_closed`、`checkpoint_writable`、`disk_free_mb`，后五项各带 `ok` 和 `reason`。网关不拆窗口、不跑 LangGraph，健康检查不调用模型。
- **HTML 入口层 `server/entry.py` 在 127.0.0.1:8011**（投影 + 驱动）：`/api/runs`、`/api/run/<id>`、`/api/stream/<id>`（SSE tail 事件文件，`id:` 是行号）、`/api/tree?path=`（只列一层，屏蔽 Binaries/Intermediate/Saved/DDC/.vs）、`/api/health`、`/api/questions/<id>`，加纯函数 `POST /api/preview`（装配预览）与两个写面 `POST /api/run`、`POST /api/answer`。静态页 `server/static/entry.html` 的状态表与 `agent/statuses.py::WINDOW_STATE_LABELS` 由 `eval/check_observer_labels.py` 钉成同一份，`entry._runs_day` 与 `run_windows._runs_day` 同源同口径。真机验收（20261116）：`POST /api/run` 起 `编译工程`，**432ms** 后 `/api/runs` 出现新 `run_id`，run 成功、退出码 0；`:resume latest` 的请求文件是 `:resume latest` + `:quit` 两行，被 REPL 逐条消费后退出。
- **思考在 L1，归地和裁判在代码**：句子 → 动作是 L0 手写规则 / 已学句式 → L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径，asset 槽位走索引）→ L3 学习回写。门只认整句（2026-11-11 起）：L0 / HUD 规则 / 已学句式命中前先过 `_clauses_accounted`，接不住的半句交回模型；L1 goals 盖不住（`unresolved` 非空或半句没人接）且 `UE_SOLVER_ENABLED=true` 时直接进求解兜底（`solver-fallback`），不再叫规划者重点一遍动作表，关着停在缺口；方案预演里有写 C++ 或改关卡才出任务书人审一次，任务书引用这份方案。只读诊断句由 `agent/diagnose.py` 归到唯一的 Automation 过滤器，收成已有的 `run_automation_test`，不扩动作表。`backend=monolith` 时蓝图类动作对同一目标最多修 3 次。`build` / `test` 的裁判 `core/judge.py` 不带 LLM，成功与否不由模型宣布。零窗口范围补窗失败且 `UE_SOLVER_ENABLED=true` 时补一扇受控求解扇 `solve`（`agent/solver.py`，预算闸门 + automation 收口，缺省 false；一个任务书最多一扇，全部未覆盖交付合并进去；省略过滤器时从真测试清单补选一次，补扇对不上未覆盖交付就不补，台账 `done` 要有改动证据）；`apply_spec` 写完源码强制编译（跨进程编译锁），错误原文进下一轮，编译成败都回连编辑器（`agent/build_error.py`）；范围有窗但交付项没被窗口接住时做覆盖对账，求解关着记 `partial_gap`、交付验收判 False（`agent/brief.py`）。
- **检索不进规划者**：`retrieve` 只为路由与句子服务；专有名词唯一命中即停，否则 BM25 + bge RRF 合并，热路径无重排。
- **决策落盘**：每次模型调用记 `data/costs.jsonl`；事件流 `context_slice` 记 `user / judge_fail / tool_kept / tool_dropped` 四个计数。

## 进度状态

做到哪一步，看 `RAG/ue-rag/data/harness/progress.json`；某功能真完成没有，看同目录 `feature_list.json`。两份都是机器可读状态，会话开工从它们重建上下文，不再从长文找进度。

本概览过期时以 `feature_list.json` 为准。

## 2026-10-29 新增：确定性玩法链（零模型）

FPS HUD 整句（`打开编辑器，在 FPS 关卡加一个 UMG 到视口…`）现在由 `agent/fps_hud.py: hud_windows` 直接拆成五扇固定因果链，`plan_windows` 标 `source=hud-rules`，`run_windows._needs_brief` 对其跳过任务书人审门；控件树由 `build_fps_hud` 用 monolith `ui.*` 原语确定性落地（不再 discover 猜 action），`check_fps_hud` 只读收口七个合同控件名（2026-11-04 起含火箭弹 `txt_RocketAmmo`）+ 父类 `FpsHUDWidget` + C++ 挂视口入口。实测 run `20261029-123907`：5 扇全绿、退出码 0、`data/costs.jsonl` 零新增（0 token）。2026-11-11 起这条链只认整句：句子里还有它接不住的半句（如滑铲）时 `hud_windows` 返回 None，交回 L1 / 求解兜底，不再把玩法半句静默丢掉。

这是「复杂玩法需求低 token」的第一个完整样板：能写死因果链的句子不叫模型，模型只处理真正有裁量的部分。残差（控件事件图绑定、命名槽）见 `Plan/20261029-1240-FPSHUD与UMG桥后续升级.md`。