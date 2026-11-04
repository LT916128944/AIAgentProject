# 复合玩法动作模式（U5）

日期：2026-10-27  
对应计划：`Plan/20261027-1708-整套UE-AI工作流能力盘点与升级计划.md` U5（G7）  
范例：`implement_shooter`（FPS 射击，20261027-150134 实跑 52.9 秒 0 重试）+ `eval/check_implement_shooter.py`

## 1. 这份文档解决什么问题

`implement_shooter` 能跑通 FPS，但它是为这一种玩法单独写的一条动作。下一个玩法
（塔防、赛车、拾取计分变体）仍要人写一条新的复合动作。本文把「复合玩法动作」的
写法沉淀成**可复制的模式**：每复制一个玩法，照这七件套走一遍，`catalog.py` 只增
不删，动作表不因玩法膨胀而失控。

复合动作与单步动作的分界：**一个窗口里要发生「多个编辑器原语 + 一个玩法级验收」**，
就按本文走；单个原语能做完的（移动、放置、读参数）不适用。

## 2. 七件套

以 `implement_shooter` 为例，每件给出落点：

| # | 件 | 落点 | 范例内容 |
| --- | --- | --- | --- |
| 1 | **CATALOG 卡片** | `agent/catalog.py` 的 `CATALOG` | `implement_shooter`：slots、summary 写清「先核对关卡里真有内容，AIAgent.Fps.Hit 只证明临时世界里的命中」 |
| 2 | **DIRECT 登记** | 同文件 `DIRECT_ACTIONS` | `--task` 直接派发，不进图、不叫模型 |
| 3 | **编辑器锁** | 同文件 `EDITOR_LOCK_ACTIONS` | 会写场景的动作必须占锁 |
| 4 | **验收清单** | 同文件 `_VERIFY[action]` | `["关卡里有 FpsPawn、FpsTarget、地面和灯", "AIAgent.Fps.Hit 失败数为 0", "关卡名唯一"]` |
| 5 | **sequence 分支** | `agent/sequence.py` 的 `_run_action` 派发 + 专属函数（如 `_implement_shooter`） | 后端分支在函数内部解决（monolith 走 `editor.load_level` → `mesh.get_scene_statistics` → `author_map_settings` → `save_packages`）；解析失败 0 次 exec 原样返回 |
| 6 | **判词** | 分支返回里带 `test` + `judge_kind` | 玩法级判词只有两种合法形态：`automation`（过滤器失败数为 0）或 `editor_assert`（编辑器读数与期望一致）；两者都要有——「内容在场」用 editor_assert，「能玩」用 automation |
| 7 | **离线检查** | `eval/check_<action>.py`，接进 `.github/workflows/offline.yml` | `check_implement_shooter.py` 的八组断言（见 §4） |

## 3. 数据形状

### 3.1 设计图 JSON（若玩法需要摆放）

放 `data/harness/level_layout/<日期>-<语义>.json`，由场景内容闸门（`agent/scene_gate.py`）
自动认领。形状：

```json
{
  "blueprint": "BP_名字",
  "note": "给人和评审看的一句话说明",
  "actors": [
    {"name": "实例标签（唯一）", "x": 0, "y": 0, "z": 0,
     "props": {"可选": "生成时写入的属性"}}
  ]
}
```

坐标是设计图唯一权威，执行层不猜。名字重复或缺失 → 扇失败，不挑第一个。

### 3.2 Automation 条款模板

玩法级测试在工程 `Source/AIAgentProject` 里用 C++ 写（`IMPLEMENT_SIMPLE_AUTOMATION_TEST`），
临时世界驱动，不依赖编辑器里摆好的关卡——**关卡内容在场由 editor_assert 核对，
临时世界测试只证明逻辑**。两者的分工写进 CATALOG summary 与 _VERIFY 各自的条目里，
别用一条测试同时背两件事。

### 3.3 人工条款边界

- 表现类（动画、受击、音效、视觉）留给 `manual` 条款，人绑，模型不改 `passed`。
- 「不能自动」的判断要复核：U2 复核发现 monolith `ai` 命名空间已能做行为树，
  「不可自动」的前提部分失效——复制模式时重新评估一次，别照抄旧结论。

## 4. 离线检查要钉住什么（范例的八组断言）

1. **包小**：动作提示包不超过预算（`check_packet_is_small`）。
2. **表外子句不硬绑**：认不出的半句走分解器，不编动作（`check_unknown_clause_is_not_hard_bound`）。
3. **缺名字不暂停问路**：原句没点名 Actor 时丢掉，不要变成暂停窗去问 /Game 路径。
4. **已确认缺口不被模型救**：人审确认过的缺口不再叫模型补。
5. **假模型抓得住**：complete 替身乱写时检查能红。
6. **只写安全路径**：`apply_spec` 类落盘只许写白名单文件。
7. **空关卡不算过**：临时世界测试会过，也不能把空关卡写成完成（`check_empty_level_is_not_a_pass`）。
8. **先内容后测试**：关卡里有人和靶才允许用过滤器收口；GameMode 没设上不算能玩。

新玩法的检查文件照这八组断言起骨架，玩法特有的拒绝路径（如「不能改默认 GameMode」）
另加。

## 5. 复制清单（做一个新玩法的实际步骤）

1. 玩法 C++ 逻辑 + Automation 测试进 `Source/AIAgentProject`（走 `ue-cpp-plan-skill` 的 Plan 门）。
2. 照 §2 七件套加动作；后端适配按 `core/monolith_bridge.py` 的覆盖矩阵检查
   （`eval/check_backend_coverage.py`）声明，没桥的原语要么加映射要么带原因声明。
3. 需要摆放的设计图照 §3.1 放进 `data/harness/level_layout/`，闸门自动接。
4. lexicon 加对应动词行（人维护，`data/skills/lexicon.json`）。
5. 跑一次真实 run，把 `data/runs/<id>.md` 的链接记进 `feature_list.json` 新条目的证据里；
   `passed` 只由裁判结果改。
6. 全量离线检查回归（约 40 条），红了先修存量再收工。
