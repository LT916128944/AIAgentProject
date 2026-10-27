---
last_updated: 2026-10-21
status: active
owner: 侑
---

# 架构边界与风险分级

## 允许依赖方向

RAG/ue-rag 各包只能引用下列范围（升级 2 会把本表编成测试）：

| 包 | 允许引用 |
| --- | --- |
| `config` | 标准库与第三方。不引用 `agent` / `core` / `server` / `eval` / `pipelines` |
| `core` | `config`、`core` |
| `server` | `config`、`core`、`server` |
| `pipelines` | `config`、`core`、`pipelines` |
| `agent` | `config`、`core`、`agent` |
| `eval` | 全部（检查者可引用各层） |

## 已知例外（白名单，只此两条，收在 data/harness/arch_whitelist.json）

1. `core/editor_exec.py` 引用 `agent.harness`（拒绝结果要带 `policy_snapshot`，暂不搬这个函数）
2. `server/gateway.py` 引用 `agent.understand`（`/api/structured` 包理解器，暂不拆网关）

历史：`pipelines/export_agent_skill.py` 引用 `agent.skill_prompt` 已在升级 2（2026-10-20）拆掉，常量移到 `core/project_rules.py`。`core/rag_core.py` 引用 `agent.cli_log` 已在 2026-10-21 拆掉，状态行移到 `core/console.py`，`agent/cli_log.py` 只转出这两个函数。白名单以外的新 import 一律判失败。

## 改动授权规则

一张图的原则不变：不新增边、不新增第二张业务图（`feature_list.json` 的 `second-graph` / `drop-langgraph` 均为 wont）。

## 风险三级

| 级别 | 内容 | 默认处理 |
| --- | --- | --- |
| 读 | 查询、检索、`read_source`、`query_actor`、`run_automation_test` | 自动放行 |
| 改 | 源码 diff、编辑器属性写 | 走白名单与路径锁；未经确认不写 `.h` / `.cpp` / `Build.cs`、不改蓝图 |
| 高危 | `save_assets` / `save_actor`、`StartPIE` / `StopPIE`、删文件、`UE_ALLOW_*` 为 false 时的改图 | 默认拒绝，需要任务 `save: true` / `pie: true` 或开关显式为 true |

三级名单固化在 `RAG/ue-rag/data/harness/risk_matrix.json`，`core/risk.py` 的 `tier()` 认名字；`graph_write` 展开成蓝图写工具；材质、UMG、动画、Niagara 的写工具仍由 `editor_exec._domain_blocked` 决定。本文件只写约定。

## 高风险门保留

任何升级文件的预授权不等于产品里取消这些门：写前缀工具必须出现在 `allow_writes` 里；`UE_ALLOW_BLUEPRINT` 等开关缺省 false；拒绝结果带 `policy`（当时的 `save`、`pie`、`allow_writes` 与各开关值）。检查代码由升级 2 的 `eval/check_architecture.py`、`eval/check_schema.py`、`eval/check_risk.py` 提供，本文件只写约定。