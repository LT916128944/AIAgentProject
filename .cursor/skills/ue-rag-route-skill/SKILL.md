---
name: ue-rag-route-skill
description: UE 编辑器操作先扫描 MCP 工具再调用。Actor、属性、验证、编译已有蓝图时用。UE_ALLOW_BLUEPRINT=true 时蓝图 Graph 也走这里。禁止把 830 个 MCP schema 装进上下文。
---

# UE RAG 工具路由

编辑器要动手时走这里：移动 Actor、读属性、编译已有蓝图。`UE_ALLOW_BLUEPRINT=true` 时，蓝图 Graph 也走这里，失败达到 `max_retries`（缺省 3）后停止。开关为 `false`，或要改材质图、UMG、动画蓝图、Niagara 时，走 `ue-manual-blueprint-skill`，不扫描工具。规划留在当前对话。不要把 `agent/run_agent.py` 当日常入口。闭环任务在 `RAG/ue-rag` 跑 `python agent/run_task.py --task <json>`，默认 `UE_EXEC_MODE=real`。

资产语义查询（项目里有什么、类在哪）仍走 `search_ue_project`，不走本 Skill。只改 C++ 源文件、纯问答，不扫描工具。

## 禁止

- `GetDynamicTools` 扫 unreal 命名空间
- `tool_search` / `list_toolsets` / `describe_toolset`
- 读 `data/ue_mcp_index.json`、事实库整文件、任何 Toolset 的全量 schema
- 扫描不相关时回退全量 schema

## 命令

Python：`E:\UEProject\AIWork\RAG\ue-rag\.venv\Scripts\python.exe`
工作目录：`E:\UEProject\AIWork\RAG\ue-rag`

1. 扫描。只出卡片（`full_name` / `toolset` / `tool_name` / `required` / 截断描述），没有 `input_schema`。

```powershell
python scripts/scan_mcp_tools.py --query "<自然语言>" --top-k 8
```

2. 选定一个 `full_name` 后，才导出这一个 schema。

```powershell
python scripts/scan_mcp_tools.py --schema "<full_name>"
```

3. 调用。直连 `http://127.0.0.1:8000/mcp`。stdout 约 200 字。编辑器需已开。保存关卡与 PIE 跟随 `UE_DEFAULT_SAVE` / `UE_DEFAULT_PIE`（缺省开）；用户明确说不用保存/不用运行才加 `--no-save` / `--no-pie`。

```powershell
python scripts/call_mcp_tool.py --toolset "<toolset>" --tool "<裸名>" --args "<json>"
```

要保存时加 `--save`，要 PIE 时加 `--pie`。没加则 `save_assets` / `save_actor` / `StartPIE` / `StopPIE` 会被执行器拒绝。其它写操作在没有任务文件时，允许列表只有这一次的 `--tool`。

`--toolset` 用卡片里的 `toolset`，`--tool` 用 `tool_name`。不要把 `full_name` 整段塞进 `--tool`。

## 失败

卡片明显不相关：换一种说法再扫一次。仍没有就停，只说一句未知工具。调用失败把脚本原文交回，最多重试一次。不要改去发现全部 MCP 工具。
