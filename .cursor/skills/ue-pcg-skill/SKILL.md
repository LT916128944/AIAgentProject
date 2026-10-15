---
name: ue-pcg-skill
description: PCG Graph：Node、Pin、参数、Subgraph、执行。随机必须显式 Seed。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- PCGToolset.PCGToolset (30)
- PCGToolset.PCGSpatialToolset (1)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`P`=`PCGToolset.PCGToolset`
- 图：`CreateGraph` · `GetGraphStructure` · `GetGraphSchema` · `SetGraphParams` · `SetGraphInstanceParams`
- 节点：`Add/Remove/UpdateNode` · `Connect/DisconnectNodePins`
- 跑：`ExecuteGraphInstance` · `PCGSpatialToolset.RunPCGInstantGraph`
先定：Graph 资产 vs 空间实例、阶段（Input→Filter→Transform→Grammar→Output）、Seed、输出 ISM vs Spawn Actor。随机分支必须绑显式 Seed。改一步验一次结构或点数。详版 `rg '^## PCG' references/ext_ue5_knowledge.md`。
