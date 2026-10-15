---
name: ue-niagara-skill
description: Niagara System：Emitter、Module、Renderer、Stack、参数、Niagara Blueprint。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- NiagaraToolsets.NiagaraToolset_System (46)
- NiagaraToolsets.NiagaraToolset_Component (4)
- NiagaraToolsets.NiagaraToolset_Assets (3)
- NiagaraToolsets.NiagaraToolset_Blueprint (2)
- NiagaraToolsets.NiagaraToolset_Info (1)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`N`=`NiagaraToolsets.NiagaraToolset_System` · `C`=`NiagaraToolsets.NiagaraToolset_Component`
- System：`CreateNiagaraSystem` · `GetSystemSummary` · `SetSystemData` · `SetEmitterData`
- Stack：`Add/RemoveModule` · `SetStackInputData` · `Add/RemoveRenderer`
- 验：`GetStackIssues` · `GetSystemCompileState` 各一次
- 组件：`C.SetSystem` · `C.SetVariable`
已知 System 直接改，不扫资产。先 `GetStackIssues` 再改单个模块，不整段重灌。
