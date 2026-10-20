---
last_updated: 2026-10-20
status: active
owner: 侑
---

# 记分板指标定义

只定义指标，不写采集代码。采集在升级 3（`scoreboard-file`），从已有 trace 写出记分板。

## 指标

| 指标 | 定义 |
| --- | --- |
| 任务成功率 | 裁判 ok 的任务数 / 计入分母的任务数。分母取 `eval/bench_manifest.json` 里可计入成功的任务；改关卡且不保存的不进分母 |
| 首次通过率 | 重试次数为 0 且裁判 ok 的任务数 / 同上分母 |
| P95 耗时 | 任务从开始到裁判裁决的耗时分布的 95 分位，来源 trace |
| 每次成功的 prompt token | 成功任务累计的 prompt token（含重试），来源 `data/costs.jsonl` 与事件流 |
| 工具拒绝次数 | 因 `save` / `pie` / `allow_writes` / 开关被拒的调用次数，来源拒绝结果里的 `policy` |

## 落地字段（eval/report.py → data/harness/scoreboard.json）

`compute_scoreboard(rows)`：`tasks` / `ok` / `first_pass` / `prompt_tokens_per_ok` / `tool_refused` / `source`。输入是已有 trace jsonl 或 `data/traces` 目录；没有 usage 的字段留空 null，不填 0，不估算金额。`first_pass` = 该任务 ok 且 retry 为 0，且没有拒绝事件。`tool_refused` 以 `failure_class=tool_refused` 为准，没有分类时按 trace 拒绝事件文本计。

## 安全越界是硬门槛

保存、PIE、越权写被拒绝时**记一次拒绝**，不把拒绝记成任务成功；被拒绝后任务即使后续步骤全 ok，也不计入首次通过率。

## 判定口径

- 「成功」只认裁判或离线检查，不认模型宣布（与 `feature_list.json` 的 `passed` 同一口径）。
- 人工条款未过、自动条款过了的任务是「部分完成」，不进成功率分子。
- 指标随每次评测输出成静态页/JSON 文件，不部署常驻监控站。