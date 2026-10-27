---
name: ue-asset-skill
description: Asset：查找、加载、保存、exists、Content 路径。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.asset.AssetTools (21)
- editor_toolset.toolsets.data_asset.DataAssetTools (1)
- editor_toolset.toolsets.data_table.DataTableTools (10)
- editor_toolset.toolsets.curve_table.CurveTableTools (9)
- editor_toolset.toolsets.string_table.StringTableTools (8)
- editor_toolset.toolsets.texture.TextureTools (2)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`AssetTools`：`find_assets` / `exists` / `load_asset` / `save_assets`。路径写全 `/Game/X.X`。未知路径用最小条件 `find_assets`，不扫整个 Content。
写场景之后保存关卡是缺省行为（`UE_DEFAULT_SAVE`，缺省开）；用户明确说不用保存才只改内存。保存只写目标路径，不用空列表（会存全部脏资产）。
关卡内 Actor 改动 = 存关卡：`SceneTools.get_current_level` → `save_assets({asset_paths:[level]})`。禁 `save_actor`。
PIE 中 `exists`/`save_assets` 报不存在 = 资产被锁，先 StopPIE。`create` 已存在：先 `exists`，别用报错试错。
