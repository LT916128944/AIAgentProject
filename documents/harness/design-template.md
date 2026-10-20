---
last_updated: 2026-10-20
status: active
owner: 侑
---

# 功能设计模板

新功能开工前填这份模板，落 `Plan/` 或任务书。字段只有下面六个，不加长文叙述。

## 模板

```markdown
# <功能名> · 设计

- Status: draft | active | done | dropped        # 与 feature_list.json 对应的 id 用 status/passed 表达
- 目标：<一句话。达到什么可判定状态>
- 非目标：<明确不做的事，至少一条>
- 涉及路径：<要改的文件/资产，逐行列；没有的路径不列>
- 验收标准：<每条必须能被裁判（judge/build/test）或离线检查判定；模型口头「已完成」不算>
- 依赖：<依赖的 feature_list id、开关（UE_ALLOW_*）、外部进程>
```

## 填写规则

- `验收标准` 是硬字段：写不出裁判或离线检查能判的句子，就回到 `目标` 重新拆。
- `涉及路径` 决定授权范围：跑动时越出此清单的写入视为越界。
- `非目标` 至少一条，防止范围漂移。
- 完成的唯一改写路径：裁判或离线检查通过 → 改 `feature_list.json` 对应 id 的 `passed: true`、`status: done`。设计文档自身不携带「已完成」效力。

## 已放弃

放弃的功能把 id 标 `wont`（`feature_list.json`），`notes` 写放弃原因，不删除设计原文。