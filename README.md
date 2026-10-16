# AIAgentProject

UE 5.8 工程，给 AI Agent 一个可操作的编辑器和一份 C++ 游戏模块。检索、对话和执行闭环在旁边的 [ue-rag](https://github.com/LT916128944/ue-rag)。

| 文档 | 管什么 |
| --- | --- |
| 本文 | 仓库里有什么、怎么和 ue-rag 放在一起 |
| [`WORKFLOW.md`](WORKFLOW.md) | 已经接上的对话、检索、裁判和评测 |
| [`周计划.md`](周计划.md) | 学习路线、岗位对标、13 周计划 |
| [`AGENTS.md`](AGENTS.md) | Cursor 里的工具路由和 Token 节流 |

## 目录怎么放

`start.bat` 按相对路径找上一级的 `RAG\ue-rag`。两个仓库克隆之后要保持这个位置：

```text
AIWork\
├── AIAgentProject\          本仓库
└── RAG\ue-rag\              https://github.com/LT916128944/ue-rag
```

## 这个仓库里有什么

- `AIAgentProject.uproject`：引擎关联 5.8。已启用 `ModelContextProtocol`、`MCPClientToolset`、`AllToolsets`、`EditorToolset`。
- `Source/AIAgentProject/`：游戏模块。巡逻敌人的 C++ 半边是 `ATestEnemy` 和 `UTestEnemyPatrolComponent`，发现玩家、追击、近战各留一个 `BlueprintImplementableEvent`，图由人接。旁边还有测试用的 Pawn、Character、GameMode、PlayerController。
- `Content/`：测试关卡和蓝图，含 `BP_Gun`、`BP_Sphere`、`BP_Character`、`BP_GameMode`、`Maps/L_NewTest`。
- `.cursor/`、`.codex/`：域技能和规则。域技能正文以 `.codex/skills` 为源，同步到 `.cursor/skills`。
- `documents/`：功能设计。导出后进入 ue-rag 的检索。
- `Plan/`：过程记录，不进入检索。
- `start.bat`：转到 `RAG\ue-rag`，打开三条对话。

## 准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`。官方 Unreal MCP 需在 `http://127.0.0.1:8000/mcp`。编辑器没开时，写场景和改蓝图都会失败。
2. 按 [ue-rag 的 README](https://github.com/LT916128944/ue-rag) 准备 Python 3.12 虚拟环境、依赖和 `.env`。至少填 `DEEPSEEK_API_KEY`。密钥不要提交。
3. 第一次使用前的入库、工具索引和三条对话的差别，按 [`WORKFLOW.md`](WORKFLOW.md) 的「拉取之后先准备」。

准备好之后双击 `start.bat`：

| 选项 | 行为 |
| --- | --- |
| 1 普通对话 | 检索后回答。不改工程，不编译 |
| 2 单循环 | 一句话拆成步骤，每步走同一张图 |
| 3 多窗口 | 分层规划后分扇执行，再集成 |
| 4 续接 | 续接最近一个没跑完的多窗口 run |

蓝图、材质、UMG、动画蓝图、Niagara 的连线默认不改。要改蓝图图时，在 ue-rag 的 `.env` 里把 `UE_ALLOW_BLUEPRINT` 设为 true。未要求则不保存关卡，也不 StartPIE。

## 现在做到哪

检索、改 C++、编译、读错误、再改、再测这条窄闭环已经能对固定任务做编译和断言。目标句「一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人」还做不到：C++ 半边在，行为树、动画和受击表现仍由人在编辑器里绑。

细节和完成度以 [`WORKFLOW.md`](WORKFLOW.md) 为准，不要用 [`周计划.md`](周计划.md) 第 4.3 节那张开工时的缺口表判断当前仓库。
