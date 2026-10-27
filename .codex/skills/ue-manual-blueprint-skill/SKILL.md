---
name: ue-manual-blueprint-skill
description: >-
  蓝图改图先看 UE_ALLOW_BLUEPRINT。false 时不改图，把连线步骤写入 Plan/。
  true 时才改蓝图，失败达到 max_retries 后停止。
  Use when wiring a Blueprint graph, 蓝图节点, BeginPlay, 材质图, UMG, 动画, Niagara.
---

# 蓝图开关

先在 `RAG/ue-rag` 用该目录的 Python 打印开关。不要读 `.env`。

```powershell
python -c "from config import allow_blueprint; print(allow_blueprint())"
```

## false

不调用 `write_graph_dsl`、`create_node`、`connect_pins`、`add_event`、`set_pin_value`、`add_variable`。

把操作步骤写入 `Plan/YYYYMMDD-HHMMSS-蓝图名.md`，然后停。文件要写出：

1. 目标，用用户原话。
2. 打开哪张蓝图，进入 Event Graph。
3. 从哪个事件的哪根引脚连到哪个节点。例如：Event BeginPlay 的白色执行引脚连到 Delay，Duration 填 0.1，再连到 Set Material。
4. 编译通过即可。

`--query` 在开关为 false 时也会自己写这份文件。已经有同一次的文件就改那一份，不要再新建。

## true 且后端是 monolith

不写人工连线清单来代替执行。同一张蓝图、同一个目标走 Monolith：还不知道 action 才 discover，然后 describe 一个 action，然后 exec，再做编译或只读复查。这一串算 1 次。复查通过就停。最多 3 次。第 3 次仍失败就停，原因写明「已尝试 3 次，Monolith 不具备该能力」和最后一次错误原文。不改连官方 8000，不换另一张蓝图，不把失败改写成待办。

`UE_ALLOW_BLUEPRINT=false` 时立刻拒绝，次数记 0，不出现上面这句止损。

## true 且后端不是 monolith

改走 `ue-rag-route-skill` 和 `ue-blueprint-skill`。先读图，再最小改，再编译。不保证一次做对。同一目标失败达到 `max_retries`（缺省 3）后停止，不要换一张蓝图。

## 其他图

材质图、UMG、动画蓝图、Niagara 各看一个开关，缺省都是 false。变量在 `RAG/ue-rag/.env`：`UE_ALLOW_MATERIAL`、`UE_ALLOW_UMG`、`UE_ALLOW_ANIM`、`UE_ALLOW_NIAGARA`。空着或不是 `true` 时，在对话里给出步骤后停，不改图，不写 `Plan/`。对应变量是 `true` 时，写工具才放行。读工具不受这四个开关影响。
