---
name: ue-static-mesh-skill
description: Static Mesh：Nanite、LOD、Collision、材质槽、导入。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.static_mesh.StaticMeshTools (16)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`M`=`editor_toolset.toolsets.static_mesh.StaticMeshTools`
Nanite：`set/is_nanite_enabled` · LOD：`generate_lods`/`set_lod_thresholds` · 碰撞：`generate_convex_collisions`/`remove_collisions` · 材质：`set/get_material` · 查：`get_bounds`/`get_lod_count`
材质槽/碰撞会影响所有引用该 Mesh 的地方。生成 LOD/碰撞不可逆：用户没要求不做。改完用对应 Get 回读一次。不批量改全部 Mesh。
