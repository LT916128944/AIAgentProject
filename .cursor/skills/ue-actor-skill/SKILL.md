---
name: ue-actor-skill
description: 场景 Actor：移动/Transform/组件/Tag/Label、按名查找。挂接见 attach.md。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.actor.ActorTools (17)
- editor_toolset.toolsets.scene.SceneTools (20)
- editor_toolset.toolsets.primitive.PrimitiveTools (4)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`A`=`editor_toolset.toolsets.actor.ActorTools` · `S`=`editor_toolset.toolsets.scene.SceneTools`

- 找：`S.find_actors` 必填 `{"name":"Cube","tag":"","collision_channels":[]}`（空也要传）。PIE 实例加 `actor_type={"refPath":"/Game/X.X_C"}`
- 写：`A.set_actor_transform` `{actor:refPath, xform, worldspace=true}`；未给的 xform 字段不改
- 读：`A.get_actor_transform` 一次即停。禁无关 `get_label`/`get_components`
- 存（仅用户要求）：同脚本 `S.get_current_level` → `AssetTools.save_assets({asset_paths:[level]})`。`S.save_actor` 对关卡内 Actor **必报** `is not an external actor asset`
- 组件：`A.get_components` 必须传实例或 CDO `/Game/BP_X.Default__BP_X_C:<成员>`，传蓝图资产报 `is not valid Actor`

`IsPIERunning=false` 时操作的是当前已加载关卡，不要为此 StartPIE。挂接到 Pawn：读 [attach.md](attach.md)。
