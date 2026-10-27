---
name: ue-dsh-verify-skill
description: 说完成、修好、通过或已生成之前用。先 read_contract 再 verify，只引用返回的 ok 与 kind。manual 条款不碰。
---

# DSH 先证后说

触发：任何「完成 / 修好 / 通过 / 已生成」之前。

1. `read_contract(run_id)` 取条款。只认 `passed` 和 `evidence_kind`。
2. 对每条 `passed: false` 且不是 `manual` 的条款调用 `verify(run_id, criterion_id)`。
3. 引用返回的 `{ok, kind, summary}`。`ok: false` 就报未完成，并附错误原文（不超过 240 字），不得换一种说法。
4. 连续 2 条 verify 失败就停。把剩下的 unmet 列表交给人，并交给 `run_windows`。
5. `manual` 条款永远不碰。`verify` 不能把 `manual` 改成通过。

没有 `{ok: true, kind}` 的条款就是没完成。模型写出的「已完成」不改变任何 `passed`。
