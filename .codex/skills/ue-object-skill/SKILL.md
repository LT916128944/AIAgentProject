---
name: ue-object-skill
description: UObject 属性：读、写、类查询。实例/CDO/组件路径不可混用。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.object.ObjectTools (6)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`O`=`editor_toolset.toolsets.object.ObjectTools`
`get_properties` / `set_properties` / `get_class`。属性名不明才一次 `list_properties`。
`values` 是 JSON 字符串：`'{"StaticMesh":"/Engine/BasicShapes/Cube.Cube"}'`。
实例 ≠ CDO `/Game/BP_X.Default__BP_X_C` ≠ 组件 `:Mesh` / `:_GEN_VARIABLE`。改 CDO 后要 `compile_blueprint`。改完回读一次。
