---
name: ue-monolith-backend-skill
description: UE_EDITOR_BACKEND=monolith 时的编辑器入口。discover、describe、exec，不扫官方 830 个工具。
---

# Monolith 后端

先看 `RAG/ue-rag/.env` 的 `UE_EDITOR_BACKEND`。只有 `monolith` 才用本技能。未写该键或值为 `official` 时走 `ue-rag-route-skill`，不要调用下面三条命令。

工作目录 `RAG/ue-rag`，Python 用该目录 `.venv`。

```powershell
python -c "import config; print(config.editor_backend())"
python scripts/monolith_backend.py --discover
python scripts/monolith_backend.py --discover --namespace blueprint
python scripts/monolith_backend.py --describe --namespace blueprint --action get_graph_summary
python scripts/monolith_backend.py --exec --namespace mesh --action get_actor_info --args "{\"actor_name\":\"Cube\"}" --no-save --no-pie
```

禁止 `detail=true` 和 `verbose=true`。禁止 `scan_mcp_tools.py`、`call_mcp_tool.py` 和端口 8000。选错入口就停，不要改调另一边。PCG 没有命名空间，直接失败。stdout 约 200 字。同一任务最多 8 次调用。`get_graph_data` 不带过滤时拒绝，改用 `get_graph_summary`。

## 域卡片

每张在 `cards/<命名空间>.md`，一次只读用到的那一张。下面只是索引，没有动作表。只读优先：`get` `list` `find` `read` `search` `discover` `validate` `compare` `audit` `explain` `resolve`。`compile_blueprint` 与 `compile_widget` 允许，且不保存。拿不准的 action 当写。

| 命名空间 | 路径参数 | 写开关 |
| --- | --- | --- |
| blueprint | asset_path | UE_ALLOW_BLUEPRINT |
| material | asset_path | UE_ALLOW_MATERIAL |
| ui | asset_path | UE_ALLOW_UMG |
| animation | asset_path | UE_ALLOW_ANIM |
| niagara | asset_path | UE_ALLOW_NIAGARA |
| ai | asset_path | UE_ALLOW_AI |
| mesh | actor_name | UE_ALLOW_MONOLITH_WRITE |
| gas combograph logicdriver audio level_sequence input localization config source project editor | describe 得到的路径参数 | UE_ALLOW_MONOLITH_WRITE |

`run_python` 要任务书写明 `allow_python: true`。`save`、`pie`、`load_level` 看任务书的 save / pie。

2026-10-26 实读：`get_actor_info` 的 `transform.location` 是 `[x, y, z]`。`compile_blueprint` 成功时 `success` 为 true 且 `error_count` 为 0。
