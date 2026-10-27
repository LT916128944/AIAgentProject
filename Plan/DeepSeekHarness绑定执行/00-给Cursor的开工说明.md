---
id: dsh-exec-00
priority: 100
pre_authorized: false
ask_user: true
stop_to_confirm: false
session: 1
depends_on: []
last_updated: 2026-10-23
status: ready
owner: 侑
---

# 00 · 给 Cursor 的开工说明（DeepSeek Harness 绑定执行包）

这套卡把 `Plan/20261023-1520-DeepSeekHarness深度绑定.md` 拆成 Cursor 一次能吃完的工作单。**主计划仍是设计与验收的总纲；执行只读当前这张卡**，不要整份读主计划，也不要读 `WORKFLOW.md`、`.analysis/` 底稿、`ue_mcp_index.json`。

## 1. Cursor 能做 / 不能做

| 能做（本执行包范围） | 不能做（留给人） |
| --- | --- |
| 改仓库内代码、配置模板、技能、文档 | 在 DSH 里点技能/工具列表、看会话 token 面板 |
| 跑离线检查（`eval/check_*.py`，mock，不调模型） | DSH 真机三步：检索、写拒绝、verify 改合同 |
| 生成 `dsh/` 模板、`check_dsh_*.py`、`agent/dsh_cli.py`、`server/dsh_bridge.py` | 启动/重启 DSH、改 DSH 设置、导出会话日志 |
| 跑 P0 基线、P4 写锁测试、P5 fixture 导入 | 真机编译/关编辑器/保存关卡/PIE |
| 把 profile 模板写成仓库文件并校验结构 | 直接改 `C:\Users\X\.dsh`（除非人明确让你复制；默认留给人） |

> Cursor 的 shell 是 IDE 终端；不要尝试命令行启动 DSH（本机没有独立 CLI）。所有 `dsh` 相关真机验证都由人做。

## 2. 执行纪律

1. **一卡一会话**。开工只读：本卡、`AGENTS.md`、卡里点名的文件；说完「完成」前必须跑完本卡验收命令。
2. 每卡 `pre_authorized: false`。开工前由侑把本卡改成 `true`（或在对话里明确批准），否则只做只读检查并停。
3. 失败最多修一次。第二次仍失败：把命令原文与输出写进本卡文末「执行记录」，停下，不扩大改动。
4. 不改 `Source/`、`Content/`、`Saved/`、`Binaries/`、`Intermediate/`；不改 `.env` 的值；不 `git commit` / `git push` / 改 git config。
5. 不新增第二张业务图、不改 LangGraph 边、不让模型文本改 `passed`、不把 830 个 MCP schema 拉进任何上下文。
6. Python 一律用 `E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe`；工作目录 `RAG/ue-rag`。
7. 每次改完，在最终回复里给：改动文件清单（绝对路径）、验收命令与输出尾行、未完成事项、需要人做的真机项。

## 3. 与 `.cursor/rules` 的关系

`.cursor/rules/ue-cpp-plan.mdc` 要求「改 UE 项目先写计划并确认」。本执行包**不改 UE 工程源码**，只改 `RAG/ue-rag` 代码、`.codex/.cursor` 技能、文档与配置；且本卡已列允许写入。冲突时本卡优先，但**不得**顺手改 `Source/` 或蓝图。

`.cursor/hooks/refuse_graph_writes.py` 是 Cursor 自己的写图拦截。P2 会把它改成薄包装，改完必须用第 02 卡的 hook 自测命令确认 Cursor 侧仍能拒绝。

## 4. 人工门（每卡结束要把这些列给人）

- 把仓库里的 `dsh/cordis.patch.yml` 条目追加到 `C:\Users\X\.dsh\profiles\desktop\cordis.patch.yml`，重启 DSH；
- DSH 里确认技能目录出现 `ue-dsh-*`，工具列表只有 `mcp__uerag__*`（≤9 个）；
- DSH 里跑真机三步（见主计划验收矩阵 15–18）；
- 记录 DSH 会话 token 用量（P4/P5 要填数）。

## 5. 卡清单与依赖

| 卡 | 内容 | 依赖 | 预计 |
| --- | --- | --- | --- |
| `01-P0-P1-基线冻结与静态绑定.md` | P0 基线 + P1 配置/技能/预检/检查 | 无 | 1.5 天 |
| `02-P2-写闸单源与MCP网关.md` | P2 写闸单源、快照独立、证据 CLI、MCP 网关 | 01 | 3–4 天 |
| `03-P3-技能与合同.md` | P3 三个技能、verify 映射、合同检查 | 02 | 3 天 |
| `04-P4-编排与交接.md` | P4 只读 workflow、handoff 校验、写锁、A/B | 03 | 5–7 天 |
| `05-P5-P6-观测成本与文档收口.md` | P5 成本导入 + P6 文档/CI/feature 收口 | 04 | 3 天 |

## 6. 执行记录模板（回写到本卡与主计划）

```text
YYYY-MM-DD Pn
- 改动：<文件列表>
- 命令：<命令> → <输出尾行>
- 结果：通过 / 失败（第几次）
- 遗留：<无 / 具体项>
- 需要人做：<DSH 真机项 / 无>
```
