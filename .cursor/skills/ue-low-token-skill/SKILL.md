---
name: ue-low-token-skill
description: UE5.8 MCP 未知工具集。跨域、索引未覆盖的 Toolset、发现失败时用。路由见 AGENTS.md。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
未见专属 Skill 覆盖的工具集（按需直接查索引）：
ToolsetRegistry.AgentSkillToolset / AutomationTestToolset.AutomationTestToolset / DataflowAgent.DataflowAgentToolset
DataRegistryToolset.DataRegistryTools / EditorToolset.EditorAppToolset / GameplayTagsToolset.GameplayTagsToolset / GameFeaturesToolset.GameFeaturesToolset
GASToolsets.GameplayCueToolset / GASToolsets.AttributeSetToolset / GASToolsets.AbilitySystemInspectorToolset
PhysicsToolsets.PhysicsAssetToolset / PluginToolset.PluginToolset / SemanticSearchToolset.SemanticSearchToolset
WorldConditionsToolset.WorldConditionTools / state_tree_toolset.toolsets.state_tree.StateTreeTools
conversation_toolset.toolsets.conversation.ConversationTools / aimodule_toolset.toolsets.behavior_tree.BehaviorTreeTools

禁 list_toolsets / describe_toolset，除非本清单/索引都没有。
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->


索引没有：`tool_params.ps1 -Search k`；仍无 → 按 AGENTS 求助。`call_tool` 不派发 → `mcp_bridge.ps1`。新事实回写 `ue_mcp_verified.json`（只 `rg`）。C++/存档/性能/架构：`rg -n -A 20 '^## <标题>' references/ext_ue5_knowledge.md`。
