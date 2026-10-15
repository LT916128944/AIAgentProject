---
name: ue-blueprint-skill
description: Blueprint Graph：节点、连线、变量、事件、Graph DSL、编译。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.blueprint.BlueprintTools (53)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`B`=`editor_toolset.toolsets.blueprint.BlueprintTools`

- 图：`B.read_graph_dsl` / `write_graph_dsl`（根只能 `(event …)` / `(fn …)`；输入事件不能当根）
- 节点：`create_node(graph,type_id,pos)` → `get_node_infos` 原样回传 `pin_id` → `connect_pins(output,input)`；入口 `add_event(event_name="ReceiveBeginPlay")`；默认值 `set_pin_value`
- 变量：`add_variable` · 编译：`compile_blueprint`（成功返回 null）。`write_graph_dsl` 会自动编译
- 建资产：`B.create(folder_path,asset_name,asset_type={"refPath":"/Script/Engine.Actor"})`；组件走 ActorTools + `ObjectTools.set_properties`

流程：读目标图一次 → 最小改 → 编译一次。type_id CamelCase 无空格（`LeftMouse`）。关卡蓝图 `:LevelScriptBlueprint` 不是 Blueprint 资产。向量/枚举/整数/类路径一律字符串。常用接线表：`ue_mcp_verified.json`。
