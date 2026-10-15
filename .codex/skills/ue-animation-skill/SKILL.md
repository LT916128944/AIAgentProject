---
name: ue-animation-skill
description: Animation / Control Rig / Skeletal Mesh：骨骼、Socket、Rig Pin。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- animation_toolset.toolsets.controlrig.ControlRigTools (44)
- animation_toolset.toolsets.controlrig_sequencer.SequencerControlRigTools (72)
- editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools (22)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`R`=`animation_toolset.toolsets.controlrig.ControlRigTools` · `S`=`editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools`
Rig：`list_pins`/`set_pin_value`/`connect_pins`/`get_pin_value`
骨骼：`get_bone_names`/`get_skeleton` · Socket：`add_socket`/`set_socket_transform`/`get_socket_transform`
先一次 `get_bone_names` 或 `list_pins` 确认名，之后不再查。Sequencer 里的 Control Rig 用索引中的 `SequencerControlRigTools`。骨骼/Socket 改动影响所有引用该骨架的资产：没要求只读。验证回读一次。
