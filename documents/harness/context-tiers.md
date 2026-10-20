---
last_updated: 2026-10-20
status: active
owner: 侑
---

# 上下文三层

## Tier 1 · 常驻

每次会话都在，总量被刻意压小：

- `AGENTS.md`（根目录唯一权威：地图 + 九条铁律 + 路由表，≤120 行）
- `.cursor/rules/*.mdc` 四条短触发器：`ue-rag-route`、`ue-history`、`ue-cpp-plan`、`ue-harness-session`

`WORKFLOW.md` 和 `周计划.md` **不是 Tier 1**。开工状态从 `progress.json` / `feature_list.json` 重建，不从这两份长文找。

## Tier 2 · 被点名的 Skill

只读被点名的那一个域 Skill（按 `AGENTS.md` 路由表），禁止把整个 Skill 目录、整份 `ue_mcp_index.json` 或全量 MCP schema 扫进上下文。跨域一次也只取真正用到的那几节。

## Tier 3 · 按需检索

以下按需取用，不预读：

- `documents/harness/`（overview / boundaries / context-tiers / design-template / scoreboard），每份都带 `last_updated` 与 `status`，过期字段以 `feature_list.json` 为准
- `RAG/ue-rag/data/harness/feature_list.json`、`progress.json`
- `search_ue_project`（项目语料检索：资产在哪、是什么类）

## 操作细节

某条操作的具体做法：读 `WORKFLOW.md` 里对应的那一节（按标题定位），不要整份读入。历史排障：`search_ue_project` 先 `doc_kind=history` 一条，再 `doc_kind=debug` 一条。