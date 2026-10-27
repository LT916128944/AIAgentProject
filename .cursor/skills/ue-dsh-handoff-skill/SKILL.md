---
name: ue-dsh-handoff-skill
description: 多步骤或多扇任务收尾时写一份交接 JSON，并用 dsh_cli handoff-validate / handoff-merge 校验与交付。不写工具正文。
---

# DSH 交接

触发：多步骤或多扇任务收尾。

一窗一份：`data/handoffs/<run_id>/<window_id>.json`。

字段（口径同 `agent/harness.py` 的 `build_handoff`，多一个字段就校验不过）：

- `did`：不超过 200 字
- `evidence`：列表，每项 `{kind, ok, summary}`；`kind` 取 `cpp_build` / `blueprint_compile` / `editor_assert` / `automation` / `widget_graph` / `manual` / `skip`，`ok` 是布尔，`summary` 不超过 240 字
- `unmet`：字符串列表，不超过 8 条，每条不超过 240 字
- `unmet_total`、`unmet_omitted`：整数，且 `unmet_total - len(unmet) == unmet_omitted`
- `next`、`goal`：不超过 240 字
- `artifacts`：对象，每项只留 `{ref, in_level}`
- `unmet_ids`：可选，字符串列表

不写工具正文。正文里出现 `read_source` 即自检失败。

## 三步收尾

1. **写文件**（DSH 侧写到临时路径即可，例如 `data/briefs/handoff-<run>-<window>.json`）。
2. **校验**，在 `RAG/ue-rag` 运行：
   `python agent/dsh_cli.py handoff-validate --file <path>`
   返回 `{"ok": true, ...}` 才算写对；`errors` 逐条改，不要改校验器。
3. **交付**（DSH 子代理产出的单窗交接才走这条）：
   `python agent/dsh_cli.py handoff-merge --file <path> --run <run_id>`
   - 目标 `data/handoffs/<run>/<window>.json` **已存在就拒绝**，不覆盖；`--window` 缺省取源文件名。
   - 只收单窗文件；带 `windows` / `criteria` 的 run 记录会被拒。

## 边界

- 交接只是文字，**不构成证据**：`passed` 只由裁判经 `apply_judge` 改写。
- 不直接改 `run_windows` 的状态，不动 `data/runs/` 下的 run 记录。
- `unmet` 写实际没做到的，不写「已完成」；`manual` 条款留在 `unmet` 里交给人。
