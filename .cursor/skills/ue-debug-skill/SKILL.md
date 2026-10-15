---
name: ue-debug-skill
description: 排障：报错、日志、事件不触发、功能不生效、查不到对象。只读定位，不实现新功能。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- EditorToolset.LogsToolset (4)
- ConfigSettingsToolset.ConfigSettingsToolset (8)
- SlateInspectorToolset.SlateInspectorToolset (14)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



诊断只读。一次一假设，失败原文交回。日志、图、配置各读一次。结论必须带证据。

症状对不上时不要读本文件全文。`search_ue_project(现象原文, top_k=1, where={"doc_kind":"debug"})`，标题对上才用那一节。

- 日志：`LogsToolset.GetLogEntries`（`pattern` 必填，`category` 必须显式 `""`）。PrintString → `LogBlueprintUserMessages`
- 图：`BlueprintTools.list_events/list_functions/list_graphs`。关卡蓝图不是 Blueprint
- 设置：`ListCategories` → `ListSections` → `GetSectionPropertyValues`（四参全必填）
- 实况：`AssetTools.exists` · `ObjectTools.get_properties` · `ActorTools.get_components`（传实例/CDO）

取证够用即停：日志 → 入口链路 → 设置覆盖 → exists/属性。引擎内部、C++、多帧按键则停并求助。
