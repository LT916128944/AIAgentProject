---
name: ue-sequencer-skill
description: Sequencer / Level Sequence：Binding、Track、Section、Keyframe、导入导出。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- animation_toolset.toolsets.sequencer.SequencerTools (140)
- animation_toolset.toolsets.keyframing.SequencerKeyframingTools (22)
- animation_toolset.toolsets.outliner.SequencerOutlinerTools (18)
- animation_toolset.toolsets.conditions.SequencerConditionTools (9)
- animation_toolset.toolsets.custom_bindings.SequencerCustomBindingTools (8)
- animation_toolset.toolsets.import_export.SequencerImportExportTools (6)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



工具集见索引。已知 Sequence/Binding/Track 直接改；不枚举 140 个 SequencerTools。Keyframe 只动目标帧/通道。Control Rig 走 `ue-animation-skill`。
契约：Sequence 路径 → Binding → Track/Section → 帧号+通道+值。回读该 Section/帧一次。换绑定前确认旧轨道处理。
