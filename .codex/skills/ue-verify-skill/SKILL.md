---
name: ue-verify-skill
description: 最小验证：Transform/编译/资产状态。只验用户要的那一项。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.scene.SceneTools (20)
- editor_toolset.toolsets.actor.ActorTools (17)
- editor_toolset.toolsets.programmatic.ProgrammaticToolset (2)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



禁为验证而 StartPIE（除非用户要求 PIE）。成功即停。失败最多再试一次。

| 目标 | 一次证据 |
| --- | --- |
| 移动 | `get_actor_transform` |
| 挂接 | `get_components` 根 → `get_parent_component` |
| 蓝图执行 | 图内 `PrintString` 探针 → `GetLogEntries(pattern, category="")` |
| 编译 | `compile_blueprint` 返回 null |
| 资产 | `is_dirty` / `exists`（PIE 中 exists 必失败，先 StopPIE） |
| PIE 生成 | `find_actors(actor_type={refPath}, name="", tag="", collision_channels=[])` 见 `UEDPIE_*` |

不能当证据：`PressKey` 测移动、截图猜状态、逐 Actor 轮询。

PIE（仅用户明确要求）固定 4 步：`StartPIE({bSimulate:false, playMode:"PlayMode_InViewPort", warmupSeconds:3})` → Slate `Snapshot`+`Click` → 上表断言 → `StopPIE`。
