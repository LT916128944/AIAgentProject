# UE5 Skills README

[English](#english) | [中文](#中文)

## English

This folder contains Codex skills for Unreal Engine 5.6-5.8 projects.

### Skills Overview Screenshot

![UE5 Skills Overview](./assets/ue5-skills-overview.png)

> Screenshot of the Codex Skills panel showing the installed UE5 skill set and enabled status (captured on March 3, 2026).

![UE5 Skills List (AI Q&A - English)](./assets/skills-list-ai-qa-en.png)

> Screenshot of the UE5 skills list for AI Q&A (English version, captured on March 3, 2026).

### What Is Included

- Routing skills:
  - `ue5-auto-assistant`
  - `ue5-module-router`
- Domain skills:
  - `ue5-architecture`
  - `ue5-blueprint-workflow`
  - `ue5-cpp-gameplay`
  - `ue5-debug-validation`
  - `ue5-performance-packaging`
  - `ue5-save-load-replication`
  - `ue5-ui-umg-slate`
  - `ue5-world-interaction`
  - `ue5-pcg-building`

### UE5.8 Compatibility Status

All included skills now advertise UE5.6-UE5.8 compatibility. The domain workflows below have version-aware API anchors and actionable stage contracts:

- `ue5-pcg-building`
- `ue5-blueprint-workflow`
- `ue5-cpp-gameplay`
- `ue5-save-load-replication`
- `ue5-world-interaction`
- `ue5-ui-umg-slate`
- `ue5-performance-packaging`
- `ue5-debug-validation`

The compatibility pass includes:

- UE5.6-UE5.8 API anchors and version-specific exceptions
- UE5.8.2 source-derived module and routing indexes
- source-anchor validation passed on UE5.7.4 and UE5.8.2, with the existing UE5.6 path preserved
- UE5.8 PCG execution-source scheduler APIs
- stage contract section (decision-complete workflow contract)
- executable failure handling (`symptom -> locate -> fix`)
- CRLF-safe validation on Windows

### Key Routing Data

- `skills/ue5-architecture/references/ue5-engine-module-index-v2.csv`
- `skills/ue5-architecture/references/ue5-module-routing-table-final.csv`

### Quick Usage

Recommended flow:

1. Use `ue5-auto-assistant`
2. If module names are present, use `ue5-module-router`
3. Execute the routed target skill

Examples:

- `Use ue5-auto-assistant to design an inventory feature`
- `Use ue5-module-router for AssetRegistry troubleshooting`
- `Use ue5-cpp-gameplay to implement a replicated component`

### Validate Skill Pack

```powershell
python .\skills\scripts\validate_skills.py
python .\skills\scripts\validate_engine_anchors.py --engine-root "E:\UEVersion\UE_5.8"
```

Expected output: `Validation OK` and `Engine anchor validation OK`

### Rebuild Routing Index (Architecture Skill)

Auto-detect engine source:

```powershell
python .\skills\ue5-architecture\scripts\generate_module_index_v2.py
```

Explicit engine source:

```powershell
python .\skills\ue5-architecture\scripts\generate_module_index_v2.py --engine-source "E:\UEVersion\UE_5.8\Engine\Source"
```

### Sync To Global Codex Skills

```powershell
robocopy .\skills "$env:USERPROFILE\.codex\skills" /MIR
```

### Publish Checklist

1. Regenerate routing index files if architecture references changed.
2. Run `validate_skills.py`.
3. Confirm `SKILL.md` frontmatter and references are valid.
4. Commit only intended `skills/` changes.
5. Push and verify skill rendering in your Codex/IDE environment.

### Risk Notice

This skill pack is still in active iteration.

- Back up project data before applying generated changes.
- Validate in a test branch/sandbox first.
- Review generated Blueprint/C++ outputs before merging.

## 中文

本目录提供面向 Unreal Engine 5.6-5.8 的 Codex 技能包。

### Skills 总览截图

![UE5 Skills 总览](./assets/ue5-skills-overview.png)

> 该图为 Codex Skills 面板截图，展示当前已安装的 UE5 技能集合及启用状态（截图时间：2026年3月3日）。

![UE5 技能列表（AI问答-中文）](./assets/skills-list-ai-qa-zh.png)

> 该图为 UE5 技能列表（AI问答）中文截图（截图时间：2026年3月3日）。

### 已包含技能

- 路由类技能：
  - `ue5-auto-assistant`
  - `ue5-module-router`
- 领域类技能：
  - `ue5-architecture`
  - `ue5-blueprint-workflow`
  - `ue5-cpp-gameplay`
  - `ue5-debug-validation`
  - `ue5-performance-packaging`
  - `ue5-save-load-replication`
  - `ue5-ui-umg-slate`
  - `ue5-world-interaction`
  - `ue5-pcg-building`

### UE5.8 兼容状态

所有技能现已声明支持 UE5.6-UE5.8。以下领域工作流包含按版本区分的 API 锚点与可执行阶段契约：

- `ue5-pcg-building`
- `ue5-blueprint-workflow`
- `ue5-cpp-gameplay`
- `ue5-save-load-replication`
- `ue5-world-interaction`
- `ue5-ui-umg-slate`
- `ue5-performance-packaging`
- `ue5-debug-validation`

本轮兼容升级包括：

- UE5.6-UE5.8 API 锚点与版本差异说明
- 基于 UE5.8.2 源码生成的模块索引和路由表
- UE5.7.4 与 UE5.8.2 源码锚点校验通过，并保留既有 UE5.6 路径
- UE5.8 PCG Execution Source 调度 API
- 阶段契约（保证实现可执行、可交付）
- 可执行故障处理（症状 -> 定位 -> 修复）
- Windows CRLF 环境可用的校验流程

### 推荐使用流程

1. 先用 `ue5-auto-assistant`
2. 识别到模块名时，使用 `ue5-module-router`
3. 执行目标技能

### 校验

```powershell
python .\skills\scripts\validate_skills.py
python .\skills\scripts\validate_engine_anchors.py --engine-root "E:\UEVersion\UE_5.8"
```

期望输出：`Validation OK` 和 `Engine anchor validation OK`

### 风险提示

当前技能包仍在持续迭代中：

- 应用前先备份项目
- 先在测试分支或沙盒验证
- Blueprint/C++ 自动生成结果请人工复核后再合并
