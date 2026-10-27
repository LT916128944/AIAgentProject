---
name: ue-cpp-plan-skill
description: >-
  UE 项目实施计划与完成后落盘。C++ 与蓝图都算。
  Use when adding or changing Pawn, Actor, Subsystem, Component, Module,
  Blueprint graph, Widget, or when the user says 实施计划, 新建类, 蓝图, 新功能, 重构.
---

# UE 项目实施计划

管**会改到 UE 项目的任务**怎么规划、落地、落盘。编辑器操作本身走对应 MCP 域 Skill；改完回到本 Skill 写记录。排障走 `ue-debug-skill`。

写 Plan 时读 [plan-template.md](plan-template.md)。分层默认读 [patterns.md](patterns.md)。

## 何时落盘

只在 **UE 项目被实际改动并完成** 时写磁盘：

- 算：写了游戏/插件的 `.h` / `.cpp` / `Build.cs`；或经 MCP 改了蓝图 / 关卡 Actor / UMG / Niagara / Material 等编辑器对象
- 不算：改 Rule/Skill/AGENTS、分析 `documents/`、问答、只出方案没动手

将改 UE 时先落 Plan；改完再落任务记录。都是一天一份。不要写进 `documents/`。

写 Plan 或任务记录前：若存在则读 `.cursor/memory/conventions.md`；当天文件只读末尾。禁止扫 `plans/`、`任务记录/` 的其他日期，禁止扫文件夹。计划、约定和给人看的任务记录 Cursor 与 Codex 共用 `.cursor/memory`，不要在 `.codex/memory` 再写一份计划或约定。

## 流程

1. **探现状**：锁定 **1 个**同模块范式。只读那几份。无路径则 RAG 先行。禁扫盘、禁读 `Binaries` / `Intermediate` / `.uasset`。
2. **决策**：职责放哪、仿谁、不改谁。有分叉先问。
3. **出 Plan**（仅当即将改 UE）：按 `plan-template.md` 追加 `.cursor/memory/plans/YYYY-MM-DD.md`。
   - 无文件：`Write`（`# YYYY-MM-DD` + `## 主题` + 六段 + `<!-- APPEND -->`）
   - 有文件：只 `Read` 当天 `offset=-20`，在标记前插入下一份 `## 主题`
4. **实施**：用户确认后改。C++ 能不改 `Build.cs` 就不改。新逻辑放 Component、Subsystem 或 Controller。蓝图连线先看 `UE_ALLOW_BLUEPRINT`：`false` 按 `ue-manual-blueprint-skill` 写 `Plan/`；`true` 走 `ue-blueprint-skill`，失败到 `max_retries` 停止。一件事一切片。
5. **验证一次**：只验用户要的那一项。
6. **任务记录**（仅当 UE 已改完）：追加 `.cursor/memory/任务记录/YYYY-MM-DD.md`，格式如下（对齐 `documents/任务对话导出`）。

```
# YYYY-MM-DD

## 主题（如：Pawn 基础类）

### 任务 1：新建基础 Pawn
- **目标**：一句话要做什么
- **操作**：
  - 改了哪个文件 / 哪张蓝图 / 哪几步
  - 可再列一条
- **结果**：做成了什么，或哪步没做成
- **经验**：
  - 可复用的坑或约定
  - 可再列一条

<!-- APPEND -->
```

同主题继续做：`任务 2`、`任务 3`（`Read` 当天末尾看最大序号）。换主题：新开 `## 主题`，任务号从 1 计。目标/操作/结果/经验按条列，缺的段可省，不要写成一整段散文。

「经验」下次还会用 → 追加一条到 `.cursor/memory/conventions.md`。一次性坑留在当天记录。

## 硬约束

- 没改 UE 项目 → 不写 Plan、不写任务记录
- 改了 UE 但没追加当天任务记录 → 视为未完成
- 最小侵入；先仿后造
- 「直接改」/ 单行 bug：跳过大 Plan，改了 UE 仍按上面格式追加记录
