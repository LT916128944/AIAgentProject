---
name: ue-programmatic-skill
description: Unreal MCP 批处理：≥2 步合并为一次 execute_tool_script。
---

<!-- UE_MCP_INDEX:BEGIN (auto-generated; refresh via .codex/skills/ue-low-token-skill/references/tools/regenerate_index.ps1) -->
工具集：
- editor_toolset.toolsets.programmatic.ProgrammaticToolset (2)
查参/搜名/实锤/call_tool：见根 AGENTS.md。
索引版本：2026-09-21 / engine 5.8.2（52 工具集 / 830 工具）
<!-- UE_MCP_INDEX:END -->



`execute_tool_script`：`editor_toolset.toolsets.programmatic.ProgrammaticToolset`。不要先调 `get_execution_environment`（报沙箱错才调一次）。`run()` 只回决策字段（约 200 字）。同类任务 ≤8 往返。

沙箱：只可 `import json/math/datetime/copy/re/time`；**禁 `import unreal`**。调用唯一方式 `execute_tool("全名.裸名", json.dumps(args))`。返回 `_StrictDict`：`.get(k, default)` 抛错，用 `d[k]` 或 `json.loads`。必须 `run()->dict`。`open()` 只读工程目录。单步失败 try/except 带回原文。用户要求存盘时：`get_current_level`→`save_assets`，禁 `save_actor`。

```python
import json
FIND = "editor_toolset.toolsets.scene.SceneTools.find_actors"
SETT = "editor_toolset.toolsets.actor.ActorTools.set_actor_transform"
def call(n, a):
    r = execute_tool(n, json.dumps(a))
    return json.loads(r) if isinstance(r, str) else r
def rv(x):
    try: return x["returnValue"]
    except Exception: return x
def run():
    actors = rv(call(FIND, {"name": "Cube", "tag": "", "collision_channels": []}))
    if not actors: return {"success": False, "error": "Cube not found"}
    call(SETT, {"actor": {"refPath": actors[0]["refPath"]}, "xform": {"location": {"x": 100, "y": 200, "z": 300}}, "worldspace": True})
    return {"success": True, "ref": actors[0]["refPath"]}
```
