---
name: ue-material-skill
description: Material / Material Instance：节点、连线、参数、编译、实例 override。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.material.MaterialTools (22)
- editor_toolset.toolsets.material_instance.MaterialInstanceTools (13)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`M`=`editor_toolset.toolsets.material.MaterialTools` · `MI`=`editor_toolset.toolsets.material_instance.MaterialInstanceTools`
能改实例就别动父材质。只动点名的参数；禁顺手改 StaticSwitch（变体重编译）。
- 图：`get/add/delete_expressions` · `connect/disconnect_expressions` · `connect_to_output` · `recompile`
- 实例：`list_parameters` · `set_{scalar,vector,texture,static_switch}_parameter`
流程：定位 → 只读必要 → 改 → `recompile` 或回读该参数一次。编译失败只重试一次。
