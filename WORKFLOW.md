# 工作流使用说明

给拉取本仓库的人。本文描述当前已经接上的用法和框架。2026-10-12 接上三条对话、裁判和检索。2026-10-13 把模型名收进配置，技能卡片补到 19 条，评测表开始合计 token，并放入巡逻敌人的 C++ 半边。同一天补上蓝图只读索引、角色上下文切片、一条写图 hook、失败分类文本表，以及成功后的历史和任务记录。2026-10-14 把「编译工程」「启动编辑器」和「能否拾取」拆成独立步骤：编译工程不要求唯一类，拾取只读裁决。2026-10-15 把「句子 → 动作」从正则升级成四层：L0 快路径（手写规则 + 已学句式）→ L1 理解器（一次模型调用选动作填槽位）→ L2 归地（代码解析路径、自动插创建前置扇）→ L3 学习回写，见 [`Plan/20261015-1705-自然语言理解前置与归地重构.md`](Plan/20261015-1705-自然语言理解前置与归地重构.md)。

学习路线、岗位对标和 13 周计划在根目录 [`README.md`](README.md)。那份文档是目标，不是当前操作手册。10 月 13 日第 8 节之后的缺口在 [`Plan/20261013-1310-第8节之后的缺口.md`](Plan/20261013-1310-第8节之后的缺口.md)。打包和部署只写在 [`Plan/20261013-1545-打包与部署.md`](Plan/20261013-1545-打包与部署.md)，还没执行。

## 现在有三条对话

Cursor 里按 `AGENTS.md` 说话的那条还在。人确认 `.cursor/memory/plans/` 之后，可以用 `run_task.py --query` 把改、编译、测试交给闭环。未确认不调用。这不是菜单 3，菜单 3 也不会变成 Cursor 里每句话的默认动作。

不想开 Cursor 时，双击工程根目录的 `start.bat`。四条选项共用同一份检索库和同一个编辑器执行器。`run_agent.py` 还在磁盘上，菜单不再调用它。

| | 普通对话 | 单循环 | 多窗口 |
| --- | --- | --- | --- |
| 入口 | `start.bat` 选 1，或 `python agent/chat.py` | `start.bat` 选 2，或 `python agent/run_multi.py` | `start.bat` 选 3，或 `python agent/run_windows.py`。默认带终端面板，`--plain` 纯文本；菜单 4 直接续接最近一个没跑完的 run，REPL 里 `:resume latest`（或 `:resume <run_id>`）同效 |
| 谁规划 | 检索一次后直接回答 | `plan_sequence` 的手写规则。正则能拆开的句子走这里，每一步同一张图（这一列不接理解器） | 分层规划（2026-10-15 起）：L0 快路径离线零模型——手写规则强命中（每一步都能落地才强；拆出任何暂停扇算弱命中）或 `agent/learned_routes.json` 同构句式，直接走 L2 归地；句里有「实现/逻辑/随机/功能/需求/行为」这类需求说法而没有任何改码扇接住时也算弱命中（181645 的教训：需求半句不许静默丢掉、测试不许替没实现的需求报成功）。L0 不中先过 L1 理解器（`agent/understand.py`，一次 `UE_LLM_MODEL` 调用，只从动作目录选动作、填槽位原始 token、推不出的写进 unresolved），产出经 `agent/intent.py` 验收后由 `agent/grounding.py` 归地：asset 槽位索引唯一命中直连，miss 且父类可推自动插「创建蓝图」前置扇，推不出只问一次 `r`。L1 说不出合法意图才回退老规划者（重试上限 1 次）。大任务先出任务书（超过 3 扇 / 新功能重构 / 理解器有 unresolved），人审通过才执行，范围窗口继承 L2 归地候选 |
| 改工程 | 不改 | 不再每步输入 `y`。做不了或失败时暂停，可以补一次路径 | 每扇新的 `thread_id`，消息里只有这一扇的任务句。写编辑器或 `cpp_build` 的窗口过编辑器闸门串行，只读裁决并行 |
| 验证 | 不编译 | 和以前的多 Agent 相同。模型不能自己宣布成功 | 集成不叫模型。某一扇失败只重试这一扇。任务书确认后代码追加 `integrate_verify` 跑矩阵验收 |
| 记录 | 不写 trace | 每步写在 `data/runs/YYYYMMDD/`（按日期分目录，老 run 原平铺兼容读写）。图内部 trace 仍在 `data/traces/*.jsonl` | 同一目录，按窗口分节，带上依赖和 `{ok, summary}`。另有 `data/runs/YYYYMMDD/<run_id>.json` 机读状态、`data/events/<run_id>.jsonl` 事件流、`data/archive/<run_id>/` 归档 |

目标仍是 `README.md` 里那句验收：一句话生成会巡逻、发现玩家后追击、近距离攻击的敌人。检索、改 C++、编译、读错误、再改、再测由闭环做。行为树、动画和受击表现由人在编辑器里绑。这句话还做不到。C++ 半边是 `UTestEnemyPatrolComponent` 和只负责挂载的 `ATestEnemy`：绕出生点巡逻，发现玩家后追击，近距离调用 `OnMeleeAttack`。三个事件都是 `BlueprintImplementableEvent`，图由人接。窄闭环已经能对三条固定任务做编译和断言。

`chat.py` 的 `answer`、`nodes.py` 的 `plan_node`、`supervise.py` 的 `_from_model` 都读 `config.UE_LLM_MODEL`。不设环境变量时是 `deepseek-chat`。不换供应商，不改 `base_url`。

## 一句多步

菜单 2 是单循环，菜单 3 才是多窗口。两边都还是原来那张 LangGraph，没有第二张业务图。图的边没有加。模型不能自己宣布成功。路径对不上仍然不猜。

菜单 2 用 `plan_sequence` 的手写规则把一句话收成有序步骤，能落地的才跑，绑不上的暂停。每一步跑同一张图。

菜单 3 的拆句分四层（2026-10-15 起）：模型在前只做理解和选动作，代码在中做归地和裁判，跑赢后沉淀成快路径。手写规则不删，退役成种子和 L1 失联时的兜底。

```text
用户一句话
  │
  ▼
L0 快路径（离线零模型）
  手写规则强命中 / learned_routes.json 同构句式 → 直接 L2 归地
  弱命中（拆出暂停扇）或未命中 ↓
  │
  ▼
L1 理解器（agent/understand.py，一次模型调用）
  输入 = 原句 + 动作卡片 + 3 条检索标题（1KB 上下）
  输出 = 意图 JSON：goals（动作 + 槽位原始 token + 参数）、constraints、unresolved
  只选表内动作，不编路径；说不出合法意图（重试上限 1 次）→ 老规划者（窗口清单 JSON）
  │
  ▼
L2 归地（agent/intent.py 验收 + agent/grounding.py 解析，纯代码，不猜）
  action 必须在动作表里，槽位必须已声明，表外拒收
  asset 槽位：索引唯一命中 → 直连；miss 且父类可推（或给了 /Game 路径）→ 自动插「创建蓝图」前置扇
  推不出 → 暂停问一次 r
  │
  ▼
按 depends_on 跑
  每扇 = 现有 run_task，新的 thread_id，消息只有这一扇
  写编辑器或 cpp_build 的窗口共用一把锁
  │
  ▼
L3 学习回写 + 集成（都不叫模型）
  整句集成成功 → 意图归一成句式模板（when + roles + goals 形状）写回 learned_routes.json
  下次同构句子在 L0 命中，零模型调用；unresolved 非空不回写，条目 LRU 上限 50
  全部依赖满足且裁判 ok，整句才 ok；某一扇失败只重试这一扇
```

动作表在 `agent/catalog.py`：`compile_project`、`compile_blueprint`、`edit_cpp`、`bind_blueprint`、`place_actor`、`judge_pickup`、`set_pickable`、`move_actor`。表外动作暂停，不执行，也不写进快路径。可实现指槽位已经唯一，不是指场景里一定有实例，也不是指编译已经通过。场景里没有实例、编译失败、裁决是「不能」，都不删已经记下的句式。人要作废就删 `learned_routes.json` 里的那一条。

`set_pickable` 要达成 `SetPickable(true)`：实例 `bPickable` 为真，且带 `InteractableActor`。当前工具目录里没有调用 `BlueprintCallable` 的工具，所以对实例写这两个字段，写完再读。不把「写过了」当成成功。不 PIE，不保存关卡。场景里没有这个实例则这一扇失败，依赖它的裁决不跑。裁决说「不能」是结果，整句仍然算跑完。

下次同一句式，`plan_sequence` 在开口叫规划者之前就拆开。例如先学会「给场景里的蓝图打上可交互标签，再让另一个蓝图与它交互」，下一句换成别的 `BP_` 名字，只要动作词还在、蓝图或类的个数相同，就直接得到 `set_pickable` 和 `judge_pickup`。

菜单 2 的手写规则仍是下面这张：

```text
一句话
  │
  ▼
plan_sequence 手写规则
  ├─ 编译工程          cpp_build。不改源码。失败留下原文，不进 fix
  ├─ 启动编辑器        句里写了才启动。等到 MCP，最多 120 秒
  ├─ 改唯一 C++ 类     加函数 / 修编译错误。仍可进 fix
  ├─ 编译已点名的蓝图
  ├─ 移动已点名的 Actor
  ├─ 绑定拾取者 / 物体  各自唯一。蓝图：句中路径 → 索引唯一命中 → 否则问一次 r
  ├─ 放进当前关卡      两个槽都绑上才放。缺的才放。不保存，不 PIE
  └─ 只读裁决          bAllowPickup、bCanBePicked、InteractableActor、距离 ≤ 100 uu
        │
        ▼
   能落地的步骤 → 现有图
        retrieve → plan → execute → build → test → observe
                                              └ 失败且未冻结源码 → fix → plan
```

「编译工程后启动 UE 编辑器，然后测试 ATestActor 能否被 ATestPawn 拾取」现在走成：

| 步 | 结果 |
| --- | --- |
| 编译工程 | `cpp_build` 退出码 0 才继续。不调用改源码的工具 |
| 启动编辑器 | 编译通过后启动。记录里留下启动说明 |
| 绑定拾取者 | 索引里的 `/Game/BP_TestPawn.BP_TestPawn` |
| 绑定物体 | `ATestActor` 的蓝图不在索引里。暂停，只问一次 `r /Game/...`。补上的路径若还没有资产，按该类新建再放入，父类去掉 `A`/`U`/`F` 前缀。不改图 |
| 放置、裁决 | 物体没绑上就不跑。跳过这一格也不把「没测」写成「能」或「不能」 |

裁决四条都满足才是「能」：`bAllowPickup`、`bCanBePicked`、带 `InteractableActor`、与拾取者距离不超过 100 uu。缺读数、场景里没有实例、MCP 连不上，步骤失败，不写成「不能」。不写属性，不补标签。

2026-10-14 跑过这句，记录在 `RAG/ue-rag/data/runs/20261014/20261014-121440.md`：编译成功，编辑器说明是「已启动 UnrealEditor」，拾取者绑到 `BP_TestPawn`，物体槽暂停。`Source/AIAgentProject` 的 `.h` / `.cpp` 哈希与跑之前相同。`python eval/check_routes.py` 打印 `routes ok`。当前关卡是 `L_NewTest`。关卡里已有的 `BP_TestActor` 距离 500 uu 时裁决为「不能」，原因是距离，`bPickable` 和标签与裁决前相同；拿掉标签、距离 40 uu 时裁决为「不能」，原因是没有 `InteractableActor`，`bPickable` 仍为真。验证时放进关卡的 `BP_TestPawn` 没有保存，重载 `L_NewTest` 会去掉它。标签已写回 `InteractableActor`。

## 多窗口的升级：面板、任务书、续接、矩阵（2026-10-14 下午）

按 [`Plan/20261014-1633-工作流差距与升级计划.md`](Plan/20261014-1633-工作流差距与升级计划.md) 的六段做完，全部在现有进程模型里，不动图的边，不引入新依赖（面板用已装的 `rich==15.0.0`），不给任务书生成器和规划者看工具 schema。

| 层 | 现在怎么做 | 数据落在哪 |
| --- | --- | --- |
| 预处理（任务书） | 拆出超过 3 扇，或句子里说了「新功能/重构」，或理解器把半句扔进了 `unresolved`（2026-10-15：没着落的半句正是「尚未完成」清单该记的），先出任务书：任务范围、「尚未完成的全部主要内容」清单、估时区间、依赖、外部依赖，自动带总工作量和三行免责。人审门：确认 / `e` 命令就地编辑（改完立即写回同一对 .md+.json，`e help` 看写法，手改计进 `manual_edits`，确认时记「通过，附修改」）/ 写修改意见（按意见重出下一版，记审阅轮次）/ 放弃。审阅界面是 rich 表格（范围、交付、现状、依赖、窗口、估时），不是裸 markdown。窄任务走原快路径，不生成任务书。确认后的范围窗口按 id（或唯一动作）**原样继承**快路径候选，已解析的路径、父类、依赖一并保留，生成器不再重编动作/槽位；没归类的候选窗口照跑不丢（2026-10-15） | `data/briefs/<时间戳>-<slug>.md` + 同名 `.json` |
| 观测（面板与事件流） | 菜单 3 默认 `--board`：顶上主代理区（指令、阶段、审阅轮次、「● 修改了文件」绿点），左下运行窗口区（`子代理 · 可续接`、旋转符 + 当前图节点、最近输出），右下完成区（`✓ N 已完成`，按 `E` 展开明细）。`--plain` 保留纯文本；面板不吞 stdout，echo 走订阅者，run 记录照常落盘 | `data/events/<run_id>.jsonl`，一行一事件：`run_start / intent / window_state / tool_call / judge / files_modified / window_result / integration / learned_route / index_stale / run_end`。`intent` 只存决策字段（动作名、unresolved、理解器 token），理解器被调用才写；`run_start` 的 `source` 新增 `understood`；`learned_route` 在整句集成成功且回写模板时写；`index_stale` 在本 run 真新建了蓝图资产时写，提示跑 `shortcuts\asset.bat` 重建索引（要关编辑器，由人执行，续接不重复提示）（2026-10-15） |
| 持久（可续接） | 每扇状态写穿机读 run 记录（窗口定义、thread_id、已发布工件、审阅轮次都在里面）。跑到一半 Ctrl+C 或断电，`python agent/run_windows.py --resume latest`（或具体 run id）：`done` 扇直接复用结果；中断扇用原 `thread_id` 从 checkpoint 续上下文；`paused` 且有动作的扇进人工裁决；`paused` 但没绑上动作的旧死桩扇直接跳过并写明——裁决不问它，`r` 也填不活，重拆那句话才会走新路由。「最近一次」不靠 mtime，是把平铺老文件和日期子目录里的 run 按 run_id 排出来取最新，菜单 4 和 `:resume latest` 都走这条（2026-10-15 目录改日期分层后补的）。启动时面板会提示可续接的 run | `data/runs/YYYYMMDD/<run_id>.json` |
| 容错（快照与裁决） | 会写源码的窗口在编辑器闸门内先拍 `Source/AIAgentProject` 的字节快照，失败且任务无 `keep_source` 时还原；续接前先还原旧快照，半截改动不当前提。`paused` 窗口对齐单循环的 s/c/r，只问一次：`s 停止 / c 跳过 / r /Game/目录 或 r Public/类名.h`，裁决问题和输入提示同步进面板「最近输出」，不再只印在面板底下的裸终端里（2026-10-15）。环依赖开跑前报「环：W1→W2→W1」；悬空依赖报「未知窗口 id」，不再拖到集成段 | `data/snapshots/<run_id>/` |
| 调度（只读并行） | 依赖全 `done` 的窗口进就绪集；只读裁决（`judge_pickup` / `check_pickup`）共享读闸并行，写场景/编译/PIE 独占写闸，读写互斥、写写互斥。规则窗口不再假链：按名字数据流连依赖（BP_TestActor 与 ATestActor 算同一目标），独立目标不虚连；规则拆出的测试窗口接在最近的编译/改码扇后面，上游没绑上动作、解析失败暂停时同样接上。后来补出来的 PIE 窗接在最后一扇后面，上游暂停时也不抢跑。成功扇按名字发布工件 `{ref, in_level}`，下游按槽位名订阅，放置→改名→裁决这类多物任务传得了 | 事件流里的起止时间戳可证重叠 |
| 验收（矩阵与台账） | 任务书确认后由代码追加一扇 `integrate_verify`（规划者写不出它）：重复请求幂等（重跑一次最近成功的只读裁决，结论不变才过）、拒答路径、延迟窗口、断线重连、未知态恢复，一项一个子判定；产出触及文件哈希、窗口 elapsed/retry/token 汇总和归档。失败扇无下游依赖时整句记「部分完成」，报告按范围列「A 完成；B 失败」 | `data/archive/<run_id>/`；范围级台账 `data/ledger.json` |

台账 `data/ledger.json` 只给机器读：范围级 `done / partial / pending`，由集成结果回写，任务书生成器的「尚未完成的全部主要内容」从这里取。给人的记录仍在 `.cursor/memory/任务记录/`，两边不互相替代。估算只是记录字段，代码自动带免责三行，执行层不读、不作为预算、超时不杀窗口。

REPL 里 `:quit` 退出，`E` 展开上一个 run 的每扇明细（状态、ok、elapsed、retries），`:resume latest`（或 `:resume <run_id>`）续接——误敲成 `--resume ...` 或裸写 `resume latest` 也按续接处理，别的 `--xxx` 只提示不执行，不再当任务句进规划者（2026-10-14 185550、190429 两次白烧规划者调用后补的）。`start.bat` 菜单 4 直接续接最近的 run。`eval/check_routes.py` 现在同时跑 `eval/check_upgrade.py`：离线覆盖六段（事件流与机读状态、面板渲染、任务书与人审门、续接与快照还原、环/悬空诊断、只读并行、工件、矩阵台账归档）加 build 前打转熔断与 REPL 指令，通过时打印 `upgrade ok` 和 `routes ok`，不叫真实模型、不写场景。

## 分工

核心玩法走 C++，进多 Agent 闭环：`read_source`、`apply_unified_diff`、`cpp_build`。加函数或修编译错误时失败进 `fix`，并且要求唯一的 C++ 类。句里给了唯一的 `.h` / `.cpp` 文件名时（可以带 `Source\...\Public\` 这种整段路径），目标类由文件钉死，句中提到的其余类名（如括号里的参数类型 `ATestPawn`）只当引用参数，不算第二个候选；带分隔符的整段路径不参与抽类名，路径里的目录名（如 `AIAgentProject`）不当类名。没给文件名时仍要求整句只有唯一类。「编译工程」「编译项目」「编译一下工程」并且句里没有改函数时，步骤是整份工程的 `cpp_build`，不要求唯一类，不改源码；失败留下错误原文，不进 `fix`。句里写了「启动编辑器」「打开编辑器」或「启动 UE」时，编译通过后启动编辑器并等到 MCP，最多 120 秒，不要求这次写过源码。连接超时不算已连通。句里同时在改函数时，仍走改源码那一步，不被「编译工程」这条例外吃掉。PIE 里要找的对象只认 C++ 索引里真有的类，父类是 Pawn/Character 的优先；没有 Pawn 类时句中唯一的 `BP_` 名（会被放进场景的那个）优先，一个都解析不到才退 `TestPawn`，不再拿句首第一个像类名的词去场景里找。

同一句里如果还有「创建蓝图」，但没有 `BP_` 或 `/Game/` 路径，这一步会暂停。创建路由由「新增/创建/新建动词后面直接跟 `BP_` 名、`/Game/` 路径或『蓝图』」触发：句里给了完整 `/Game/` 路径就直接绑好「建蓝图」（父类用句中「继承自」的那个类），句里说了放到场景就再绑一扇「放进场景」，都不再问 `r`；没给路径才照旧暂停等一次 `r`。「给某类新增一个属性」这类动词后面跟的不是蓝图、蓝图只是顺带要编译的，不触发创建路由，仍走改 C++ 加编译那张蓝图。「编译代码」和「编译工程」一样算整份工程的 `cpp_build`，不改源码。凡是因为不知道目录或源文件而停住的步骤，都只给一次 `r` 补路径：蓝图用 `r /Game/某目录` 或 `r /Game/某目录/BP_名字`，C++ 用 `r Public/类名.h` 或 `r Private/类名.cpp`。只写蓝图目录时，资产名按类名，例如 `ATestActor` 得到 `BP_TestActor`，父类是 `/Script/AIAgentProject.TestActor`。创建蓝图之后的「放进场景」沿用这次填的路径，位置 `(0, 0, 100)` 并贴地，不保存关卡。

没有创建动词的「把已点名的蓝图放进场景」也接上了（2026-10-15）：句里写明放到/放进场景（含「放一个到场景中」「再放一条进关卡」这类带量词和「再」的措辞）、点的是 `BP_` 名或 `/Game/` 路径就触发。索引里唯一命中的直接放；索引里没有的，句中唯一的 `BP_` 名加最多一条 `/Game` 路径，父类按 `BP_` 名从 C++ 索引推（`BP_TestEnemy` 对应 `ATestEnemy`），推不出唯一候选就暂停问一次 `r`，不猜。路径上还没有资产时按该类新建再放，不改图。放置位置：场景里已有同名实例时，读它位置后错开 50 uu 放、不贴地（不叠一起）；没有才放 `(0, 0, 100)` 贴地。这一句子里若同时有「编译代码」，编译只算整份工程那一格，不再误拆出一扇必停的「编译蓝图」。

拾取按槽位，不进 PIE，也不走 Automation。`A 能否被 B 拾取`、`B 拾取 A`、`被 B 拾取` 里，B 是拾取者，A 是物体。对不上就暂停，不猜角色。每个槽在 C++ 索引里要唯一。蓝图先用句中路径，否则用索引里父类和资产名都对得上的唯一蓝图，再没有才问一次 `r`。人给出的路径上如果还没有资产，放置前按这个类新建蓝图并编译，父类是 `/Script/AIAgentProject.TestActor` 这种去掉前缀的反射名，不改图。不按 C++ 类直接生成实例。两个槽都绑上之后，缺的实例才放进当前关卡：都缺时分别放在 `(0,0,100)` 和 `(50,0,100)` 并贴地；只缺一个时放在已有实例旁边 50 uu，不贴地，避免被地面拉开。不保存。只读裁决见上一节。已经点名的蓝图（例如 `BP_Gun`）和 C++ 类写在一句里时，先改 C++ 再编译那张蓝图。标签字符串即使以 Actor 结尾，对不上索引里的类就不算类名。加函数时允许改这个类的头文件和 cpp，先改再编译，不先把「当前已经能编过」当成做完。修编译错误仍是先看到失败再改。新逻辑拆成 Component、Subsystem 或 Controller，不堆进一个 Actor。表现钩子用 `BlueprintImplementableEvent`。

带 `cpp_build` 的任务开始前，如果 `UnrealEditor.exe` 已在运行，会先关掉它再改源码、再编译。未保存的编辑器改动会丢失。编译前若编辑器又被拉起，会再关一次，避免游戏模块 DLL 被占用。

源码改动留下且编译通过后，闭环调用 `shortcuts\cpp.bat nopause`：`export_cpp_metadata.py` 再 `ingest_assets.py`。手工要刷索引时双击 `shortcuts\cpp.bat`，它会暂停等你看结果。然后启动 `UnrealEditor.exe`。不 PIE，不保存。这次如果是我们把编辑器关掉的，任务结束也会再打开，避免编辑器一直关着。菜单 1 不关编辑器，也不启动编辑器。菜单 2 和菜单 3 在要编译 C++ 时会先关编辑器；句里写了启动编辑器、并且这一步通过后，才会再打开。

蓝图改图看 `UE_ALLOW_BLUEPRINT`（`RAG/ue-rag/.env`，缺省 `false`）。`false`：不改蓝图，把操作步骤写成 `Plan/YYYYMMDD-HHMMSS-蓝图名.md`（打开哪张图、从哪个事件的哪根引脚连到哪个节点），然后停。`true`：可以改蓝图 Graph，用 `blueprint_compile` 当裁判，不保证一次做对；失败进 `fix`，达到 `max_retries`（缺省 3）后停止。材质图、UMG、动画蓝图、Niagara 另有四个开关，缺省 `false`：`UE_ALLOW_MATERIAL`、`UE_ALLOW_UMG`、`UE_ALLOW_ANIM`、`UE_ALLOW_NIAGARA`。不是 `true` 时不改这些图。

Cursor 外壳另有一条 `beforeShellExecution` hook：`.cursor/hooks.json` 调用 `.cursor/hooks/refuse_graph_writes.py`。命令行里出现 `call_mcp_tool` 时，`write_graph_dsl`、`create_node`、`connect_pins`、`add_event`、`add_variable`、`set_pin_value` 在开关不是 `true` 时拒绝；读不到 `.env` 也拒绝。材质、UMG、动画（含 Control Rig）、Niagara 的写工具看上面四个变量，缺省拒绝；`get` / `list` / `find` / `read` / `search` / `discover` 开头的读工具不看这四个开关。闭环里的拒绝仍以 `editor_exec.py` 为准，它读同一组变量。这不把 `--query` 接进每句话。

仍自动做的编辑器事：移动 Actor、读属性、编译一张已经存在的蓝图。编译不改图。

蓝图还在引擎里。留给人的是连线和资产引用。

## 框架

```text
start.bat ─┬─ 普通对话 chat.py ─ retrieve ─ 回答
           ├─ 单循环 run_multi.py ─ plan_sequence ─ 每步同一张图
           └─ 多窗口 run_windows.py ─ 规划者/任务书/快路径 ─ 每扇 run_task（只读并行过编辑器闸门）
                                          board.py 面板 · brief.py 任务书+人审门 · ledger.py 台账
                                                            │
                    ┌─ Cursor 对话 ─ scan_mcp_tools.py ─ call_mcp_tool.py ─┐
                    │                                                      │
任务 JSON 或一扇 ───┤                                                      ├─ editor_exec.py ─ UE MCP :8000
                    │                                                      │
                    └─ run_task.py ─ LangGraph ────────────────────────────┘
                              │
                              ├─ plan_sequence  手写规则，然后 learned_routes.json
                              ├─ plan_windows   L0 规则强命中/已学句式 → L1 理解器 → L2 归地 → 老规划者兜底
                              ├─ intent/understand/grounding  意图验收、一次模型理解、asset 槽位归地
                              ├─ retrieve  路由，未命中再 BM25 + bge。不进规划者
                              ├─ plan      按 verify 选 C++ 或编辑器角色
                              ├─ execute   本地工具，或 editor_exec
                              ├─ build     blueprint_compile / cpp_build（裁判，无 LLM）
                              ├─ test      editor_assert / automation，没有则 skip
                              ├─ observe   只看裁判 ok。freeze_source 时失败即结束
                              └─ fix ─→ plan     只编译工程的步骤不进这里
```

分层：

| 层 | 模块 | 职责 |
| --- | --- | --- |
| 配置 | `config.py`、`.env` | 引擎、工程、源码根、MCP 地址、执行模式、超时、模型名 `UE_LLM_MODEL`。路径缺省仍是本机，模型名缺省 `deepseek-chat` |
| 语料 | `pipelines/export_*.py`、`ingest_assets.py` | JSONL → Chroma `ue_assets`。本次出现过的 `source_type` 会删掉不在本次 id 里的旧条目 |
| 工具目录 | `scan_mcp_tools.py`、`ue_tools` | 卡片无 schema。`--schema` 才读索引里的一个工具 |
| 执行 | `core/editor_exec.py` | Cursor CLI 和 LangGraph 的唯一编辑器出口。协议仍是 `ue_mcp_client.call_ue_tool` |
| 本地补丁 | `core/source_edit.py` | `read_source`、`apply_unified_diff`。路径锁在 `Source/AIAgentProject` |
| 裁判 | `core/judge.py` | `{ok, kind, summary, errors:[{file, line, message}]}` |
| 图 | `agent/graph.py`、`agent/nodes.py`、`agent/roles.py` | 节点和边不变。`plan` 按任务种类换提示和工具。trace 记 `role` |
| 监督者 | `agent/supervise.py`、`agent/plan_windows.py`、`agent/catalog.py`、`agent/intent.py`、`agent/understand.py`、`agent/grounding.py` | L0 手写规则（每步都落地才算强命中）和 learned_routes 先拆。弱命中/未命中过 L1 理解器（动作卡片 + 原句 + 3 条检索标题，1KB 上下），意图经 `validate_intent` 验收后 L2 归地：asset 槽位 `link_asset` 索引命中直连、类可推自动插创建前置扇、推不出问一次 `r`。L1 失败才叫老规划者（看不到工具原文），重试上限 1 次。整句集成成功且 unresolved 为空才回写模板（LRU 50 条） |
| 任务 | `agent/task_schema.py`、`agent/run_task.py`、`agent/run_windows.py`、`agent/board.py`、`agent/brief.py`、`agent/ledger.py` | `--task` 跑固定 JSON。`--query` 仍是单循环。菜单 3 每扇一个新 `thread_id`（续接的中断扇用回原 id）。退出码 0/1/130。面板、任务书、事件流、机读 run 状态、快照、矩阵、台账都在这条线 |
| 评测 | `eval/run_eval.py`、`eval/report.py`、`eval/check_routes.py` | 默认仍是三条任务。`report.py` 按任务合计 token，没有 usage 就留空。`check_routes.py` 只检查图的边。trace 在 `data/traces/`，已 gitignore |

### 图怎么走

```text
retrieve → plan → 有工具调用 → execute → plan
                 → 无工具调用且 verify 非空 → build →（通过）→ test → observe
                                                      ↘ 失败 → observe → fix → plan
                 → 无 verify → 必须调用 finish 才结束
```

`verify` 为空时，`retrieve` 才把 `finish` 放进工具列表。有 `verify` 时模型不能自己宣布成功。`judge_first: true` 的任务第一轮不调用模型，直接进裁判。只编译工程的任务同时带 `freeze_source`：裁判失败就结束，不把错误送进 `fix`，因此不会改源码。修编译错误的任务不带这个标记，仍然先看到失败再改。

重试上限来自任务的 `max_retries`，缺省 3。失败才加一。`fix` 把 `errors` 写成下一条 user 消息，清空两份裁判，迫使下一轮重新编译或断言。

build 前打转分档（2026-10-14 补）：编译还没跑过时，写码成功后连续 8 次只在 `read_source` 的，不判失败，`plan` 直接返回空工具调用，借现有路由进 `build`——写好的代码不会被快照还原掉；最近 12 次调用全是读/diff 且失败 ≥6 次且期间没有一次写码成功的，才把图干净结束，诊断写进 `last_error`。`apply_unified_diff` 的冲突报错会指出对不上的那一行和文件里的实际内容；`read_source` 一次最多 200 行；trace 和事件流的 `tool_call` 行带工具失败原文。

当前检索是三层里的前两层。专有名词（类名、`/Game/`、`error C`、`BP_`）能唯一命中就停止。否则 BM25 与 bge 各取 20 条，用 RRF 合并。短名单重打分在 22 条题集上没有提高 Recall@5，热路径不加。两个向量模型收成 `rag_core._get_model` 一个。工具检索走同一漏斗的 `ue_tools`，卡片仍然不带 `input_schema`。技能卡片仍是 JSON，路由选出域之后只在该域的几张里挑最近的一张。

`fix` 会先按错误原文检索一条 `source_type=ue_error`，没有命中就不编造历史。

`plan` 发给模型的列表按角色切开：系统提示、原始用户句、最近一条「裁判失败」、当前角色允许的最近 6 条工具结果。`state["messages"]` 仍是全量，checkpoint 和 trace 不删。`cpp` 角色的请求里不会出现 `set_actor_transform` 的工具结果。图的边没有改。

### 执行器

`exec_tool(toolset, tool, args, task)` 返回完整 `{ok, data|error}`，不在这里截断。`call_mcp_tool.py` 的 stdout 仍截到约 200 字。

拒绝规则：

- `save_assets`、`save_actor` 需要任务 `save: true`。CLI 要加 `--save`。
- `StartPIE`、`StopPIE` 需要 `pie: true`。CLI 要加 `--pie`。
- 短名命中 `Set` / `set_` / `Spawn` / `Delete` / `Add` / `Remove` / `Create` 等写前缀时，必须出现在 `allow_writes` 里，否则 `{ok:false, error:"write refused"}`。
- 读工具不拦。
- CLI 没有任务文件时，`allow_writes` 只有这一次的 `--tool`。

`UE_EXEC_MODE` 默认 `real`。只有显式设成 `mock` 才走两个假 Actor，此时裁判也返回假结果。`real` 下 `INJECT_*` 不生效。

普通 MCP 调用超时 60 秒（`UE_MCP_CALL_TIMEOUT`）。裁判和编译等待 300 秒（`UE_MCP_TIMEOUT`）。

### 裁判

| kind | 做什么 | 怎样算通过 |
| --- | --- | --- |
| `editor_assert` | 读任务指定的工具。Actor 可先 `find_actors`，命中不是 1 个就停下并列出候选 | `expect.location` 的 x/y/z 与结果相差不超过 1 uu |
| `blueprint_compile` | `BlueprintTools.compile_blueprint`，参数 `{blueprint:{refPath}}` | 客户端 `ok` 且 data 为 null 或空 |
| `cpp_build` | 先关编辑器，再 `Build.bat AIAgentProjectEditor Win64 Development -Project=... -WaitMutex` | 退出码 0，且日志里没有 `error C` / `fatal error`。最多抽取 20 条。编的是编辑器要加载的模块 |
| `automation` | 同进程内 `DiscoverTests` 一次，再 `RunTestsByFilter`，再 `GetTestResults` | JSON 里的 `failed` 为 0。解析不到失败数就是失败。单词 `passed` 不算成功 |
| `skip` | 这一侧没有声明 | `ok: true`。observe 把 skip 当成通过 |

5.8 没有可用的编译 commandlet。`UnrealEditor-Cmd.exe -run=Compile` 此前已失败，不再当候选。自动编译在关掉编辑器之后编 `AIAgentProjectEditor`。游戏目标 `AIAgentProject` 的 DLL 编辑器不加载，新的 `UPROPERTY` 不会出现在已经启动的编辑器里。Cmd 继续只负责无界面导出。编译通过并且这次留下了源码修改时，跑 `shortcuts\cpp.bat nopause` 重新导出 C++ 并入库，再启动 `UnrealEditor.exe`。编辑器工具若因连接被拒绝失败，会先尝试启动编辑器并等待 MCP 最多 120 秒，然后重试这一次调用。等不到就不编造成功。若这次任务还要编译，编译前会再次关掉编辑器。

编辑器开着且 Live Coding 启用时，不要编 `AIAgentProjectEditor`，所以编译前必须先关掉编辑器。过滤器没有命中任何测试时，`RunTestsByFilter` 报错，不是 `failed: 0`。单词 `passed` 不算成功。任务带了 `automation_filter` 且 `cpp_build` 已通过时，测试节点会跑这条 Automation。失败回到 `fix`，错误里带失败计数和测试名。

`build_node` 跑 `cpp_build` 和 `blueprint_compile`。`test_node` 跑 `editor_assert` 和 `automation`。两边都要 `ok` 才算任务成功。

### 本地补丁

C++ 不让模型直接写文件。

- `read_source(path, start_line, end_line)`：相对 `Source/AIAgentProject`，拒绝 `..`，一次最多 200 行。
- `apply_unified_diff(path, diff)`：同一路径。只接受 unified diff。旧行必须与文件一致，否则返回冲突且不写入。

任务成功、并且 diff 写过 `Source/`、又不是试探任务时，`run_task.py` 调用 `shortcuts\cpp.bat nopause` 重新导出并入库。同一轮如果重试后成功，并且 `documents/历史解决方案.md` 里还没有同名标题，就追加一节：标题是错误原文的第一句，正文只有现象和用过的工具短名。然后跑 `export_docs.py`。只有历史、没有 C++ 改动时，再单独跑一次 `ingest_assets.py`。失败任务不入库，也不编造改法。写过 `Source/` 且会留下修改时，再追加当天 `.cursor/memory/任务记录/YYYY-MM-DD.md`。`inject_typo` 且会还原的任务不写任务记录，也不重新导出。

`inject_typo` 的任务在 `finally` 里把文件字节还原，不把试探错误留在工作区。本仓库目前没有 git，所以还原不用 `git checkout`。没有重置手段时，不宣称能和手工 Cursor 对照。旁路说明在 `RAG/ue-rag/eval/tasks/side/README.md`：可重复且不改文件或自带还原的任务 3 条，离 20 条还差 17 条。默认三条不变。

### 检索里有什么

| source_type | 来源 | 谁用 |
| --- | --- | --- |
| `ue_asset` / `ue_actor` | `export_asset_metadata.py`，要关编辑器 | `search_ue_project` |
| `ue_cpp` | `export_cpp_metadata.py` 扫 `Source/AIAgentProject`。`UCLASS(...)` 括号配平，和 `class` 之间可以换行，同一文件多个类各一条 | 同上 |
| `ue_bp_graph` | 编辑器开着时 `export_blueprint_graph.py` 只读 `list_graphs`、`list_variables`、`list_events`。只留路径、图名、变量名、事件名。2026-10-13 入库 4 条 | 同上，可 `where={"source_type":"ue_bp_graph"}` |
| `ue_doc` | `export_docs.py` 切 `documents/*.md` 和 `.cursor/memory/conventions.md` 的二级标题 | 同上，可 `where={"source_type":"ue_doc"}` |
| `ue_error` | 成功重试后的错误原文 sha1 | `fix` 先查一条 |
| 技能卡片 | `export_skill_cards.py` 只读 `.cursor/skills/*/SKILL.md` 前 20 行的 name 和 description。2026-10-13 起是 19 条，含 `ue-manual-blueprint-skill`。每条仍只有 name、description、path | 不进向量库。plan 时只带一张卡片的说明 |
| 排障条目 | `documents/历史解决方案.md`（`doc_kind=history`）、`documents/排障条目.md`（`doc_kind=debug`） | 仍是 `ue_doc`。一次只取 `top_k=1`，标题对上才用 |

`Plan/` 不入库。蓝图索引导出的是图名、变量名和事件名，不是整段 Graph DSL，也不等于改图。材质、UMG、动画蓝图、Niagara 不导出图。编辑器没开时这一层跳过，不编节点名。

`search_ue_project` 的可选 `where` 传给 Chroma。不传时和原来一样。排障用 `{"doc_kind":"history"}` 或 `{"doc_kind":"debug"}`。

### 排障时进上下文的内容

历史方案和 debug 症状会变长，所以不读整份 Skill，也不读整份 markdown。

稳定留在上下文里的只有技能卡片上的 description，外加规则里的这几步：

1. `search_ue_project(query=现象原文, top_k=1, where={"doc_kind":"history"})`
2. 返回的 `metadata.title` 与现象对不上，再查 `where={"doc_kind":"debug"}`，仍是 `top_k=1`
3. 仍对不上，就当没有现成方案，走域技能，并且只读用到的那一节

新的历史现象写成 `documents/历史解决方案.md` 的一个二级标题。新的 debug 症状写成 `documents/排障条目.md` 的一个二级标题。然后跑 `export_docs.py` 和 `ingest_assets.py`。不要把案例追加进 `SKILL.md`。闭环的 `plan` 同样只在标题对上时附上那一节，最长约 1200 字。

## 仓库怎么放

```text
E:\UEProject\AIWork\
├── AIAgentProject\          UE 5.8 工程 + Cursor 规则
│   ├── AGENTS.md            Token 节流与工具路由
│   ├── README.md            13 周学习路线
│   ├── WORKFLOW.md          本文件
│   ├── .cursor\             规则、Skill、记忆。技能卡片和 conventions 从这里导出
│   ├── .codex\              域 Skill 正文的源
│   ├── documents\           功能设计。导出后以 ue_doc 进入检索
│   ├── start.bat            三条对话的入口
│   └── Source\              C++。本地 diff 只能改 AIAgentProject 这一棵
└── RAG\ue-rag\
    ├── config.py            路径和超时的唯一缺省
    ├── .env                 密钥和本机覆盖。不提交
    ├── core\                执行器、裁判、检索、MCP 客户端、diff
    ├── agent\               图、节点、run_task.py、run_multi.py、run_windows.py、catalog.py
    ├── eval\                三条任务、run_eval.py、report.py
    ├── pipelines\           导出与入库
    ├── scripts\             扫描工具、调用一个工具
    └── data\                JSONL、技能卡片、Chroma、traces
```

| 目录 | 谁读 | 干什么 |
| --- | --- | --- |
| `AGENTS.md` | Cursor | 铁律和路由。`.codex/AGENTS.md` 只是指针 |
| `.cursor/rules/*.mdc` | Cursor，始终生效 | 动编辑器先扫描工具；改 UE 项目先计划、后记录 |
| `.cursor/skills/` | Cursor；闭环只读卡片 | 域操作说明。`ue-cpp-plan-skill`、`ue-rag-route-skill` 只在这里 |
| `.codex/skills/` | 域 Skill 的源 | 改 actor / blueprint 等正文后，同步到 `.cursor/skills` |
| `.cursor/memory/` | 写计划的人和后续对话 | 跨天约定会进检索。当天计划、当天任务记录仍只给人读，闭环进程不写它们 |
| `documents/` | 人，以及导出后的检索 | 功能设计。实现某个系统时仍当需求说明读 |
| `RAG/ue-rag` | 检索、执行器、闭环命令 | 语料、工具卡片、裁判、评测 |

## 拉取之后先准备

1. 安装 UE 5.8，打开 `AIAgentProject.uproject`，启用官方 Unreal MCP，确认 `http://127.0.0.1:8000/mcp` 能连。编辑器没开时，写场景和改蓝图都会失败。
2. 用 Python 3.12 在 `RAG/ue-rag` 建虚拟环境。现成环境是 `.venv`（3.12.10）。依赖见 `requirements.txt`，其中包含 `openai`、`mcp`、`langgraph`、`langgraph-checkpoint-sqlite`。
3. 复制 `.env.example` 为 `.env`。至少填 `DEEPSEEK_API_KEY`。`UE_LLM_MODEL=` 留空就是 `deepseek-chat`。密钥只放这里。`pyvenv.cfg` 里出现过的旧密钥视为已经泄露，到服务商处作废。
4. 路径要改时，改 `.env` 里的 `UE_EDITOR_CMD`、`UE_PROJECT`、`UE_SOURCE_ROOT`、`UE_MCP_URL`。空着就用 `config.py` 的本机缺省。
5. 把项目语料 MCP 配进 Cursor（stdio，由客户端拉起）：

```json
{
  "mcpServers": {
    "mcp_ue_rag": {
      "command": "E:\\UEProject\\AIWork\\RAG\\ue-rag\\.venv\\Scripts\\python.exe",
      "args": ["E:\\UEProject\\AIWork\\RAG\\ue-rag\\server\\server.py"]
    }
  }
}
```

仓库里没有提交这份配置。工具是 `search_ue_project`、`get_project_summary`。`search_ue_project` 可以多传一个 `where`。

6. 第一次使用前入库。Embedding 是 `BAAI/bge-small-zh-v1.5`，镜像 `https://hf-mirror.com`。

```bat
shortcuts\cpp.bat       C++ → ingest。改过并留下的 C++ 由闭环自动调用，参数 nopause。双击则跑完暂停
shortcuts\asset.bat     无界面编辑器导出资产和 Actor。本工程编辑器开着会失败
shortcuts\mcp.bat       重建工具索引并写入 ue_tools
```

文档和技能卡片另跑：

```powershell
python pipelines/export_docs.py
python pipelines/export_skill_cards.py
python pipelines/ingest_assets.py
```

编辑器开着、而且不要保存、不要 PIE 时，可以再跑只读刷新。连不上就停，不编资产名或节点名：

```powershell
python pipelines/export_blueprint_graph.py
python pipelines/refresh_editor_readonly.py
```

`export_blueprint_graph.py` 写入 `data/ue_bp_graph.jsonl`。`refresh_editor_readonly.py` 不调用 `UnrealEditor-Cmd`，也不加载其他关卡。全量关卡导出仍用 `asset.bat`，而且要先关编辑器。

`asset.bat` 和 `mcp.bat` 使用的编辑器路径来自 `config.py`，不再在 bat 里写第二份绝对路径。

## 日常对话怎么走

在 Cursor 里用中文或英文说需求。`.cursor/rules` 始终生效。这条链不调用 `run_task.py`。

### 只问项目里有什么

没有确切资产路径或类名时，第一动作是 `search_ue_project(query, top_k=5)`。返回 `id`、`content`、`metadata`、`distance`。`metadata.source_type` 可以是 `ue_asset`、`ue_actor`、`ue_cpp`、`ue_doc`、`ue_error`。

这次调用只查向量库。结果空了或不相关，再退回读源文件。有确切路径或要读实时属性时，改走编辑器工具。

`get_project_summary()` 返回 `ue_assets` 的条数。

### 要改编辑器

移动 Actor、读属性、编译已有蓝图走这里。`UE_ALLOW_BLUEPRINT=true` 时，蓝图 Graph 也走这里。模型读 `.cursor/skills/ue-rag-route-skill/SKILL.md`，在 `RAG/ue-rag` 用该目录的 Python：

```powershell
python scripts/scan_mcp_tools.py --query "把 Cube 上移 100" --top-k 8
python scripts/scan_mcp_tools.py --schema "<full_name>"
python scripts/call_mcp_tool.py --toolset "<toolset>" --tool "<短名>" --args "{...}"
```

扫描只返回卡片，没有 `input_schema`。`--tool` 填短名。调用经 `editor_exec.py` 直连 `http://127.0.0.1:8000/mcp`。stdout 约 200 字。

要保存加 `--save`，要 PIE 加 `--pie`。不加则保存和 PIE 被拒绝。同一件编辑器任务最多 8 次调用。只验证点名的那一项。扫描两次仍对不上就停，不要把 800 多个 schema 拉进对话。

域 Skill 在选中工具之后再读，一次只读用到的那几个。正文以 `.codex/skills` 为源，改完同步到 `.cursor/skills`。新踩的坑追加到 `ue_mcp_verified.json`，不要整文件读入。索引 `ue_mcp_index.json` 约 1.4MB，同样不要整文件读入。

官方 MCP 索引版本记在事实库：2026-09-21，引擎 5.8.2，52 个工具集、830 个工具。

### 要改 C++，并且会落到工程里

先读 `.cursor/skills/ue-cpp-plan-skill/SKILL.md`。分层见 `patterns.md`：血量、背包、AI 放 Component、Subsystem 或 Controller。

1. 锁定一个同模块的现成类。没有路径就先 `search_ue_project`。
2. 即将改工程时，把六段计划追加到 `.cursor/memory/plans/YYYY-MM-DD.md`，等确认。
3. 确认前不写 `.h` / `.cpp` / `Build.cs`。
4. 确认后只做这一件事。表现留给 `BlueprintImplementableEvent`。
5. 人点名才验证一次。
6. 工程真的改完之后，追加 `.cursor/memory/任务记录/YYYY-MM-DD.md`。可复用的约定再写入 `conventions.md`。

### 要改图

先看 `UE_ALLOW_BLUEPRINT`。`false` 时蓝图不改，按 `.cursor/skills/ue-manual-blueprint-skill` 把连线步骤写入 `Plan/`。`true` 时走 `ue-blueprint-skill`，失败次数到 `max_retries` 就停。上面的 hook 会在 Cursor 的 shell 里拦住同一批写图工具。材质图、UMG、动画蓝图、Niagara 在对应的 `UE_ALLOW_*` 不是 `true` 时只给人工步骤。只编译一张已有蓝图走 `blueprint_compile`，不改图。

单行 bug 或明确说「直接改」：可以不写大计划，动了 UE 工程仍要补当天任务记录。改 Cursor 规则、改 Skill、只分析文档、只问答：不写计划，也不写任务记录。

已沉淀的约定：1 米 = 100 uu；速度用 uu/s。键盘字母用 `BindKey`，本引擎只有 4 参数成员函数指针重载。C++ 导出按配平后的 `UCLASS(...)` 认类，和 `class` 之间可以换行。Actor 坐标是导出当时的快照，检索正文里写「坐标不是实时位置」。

人点名的证据仍是 `ue-verify-skill`：位置用 `get_actor_transform`，蓝图编译用 `compile_blueprint` 返回 null，日志用 `GetLogEntries`。默认不 PIE。

## 命令闭环怎么跑

工作目录 `RAG/ue-rag`，Python 用 `.venv`。

一份任务：

```json
{
  "id": "bp-gun-compile",
  "query": "编译 BP_Gun 并确认没有编译错误",
  "verify": [{"kind": "blueprint_compile", "refPath": "/Game/BP_Gun.BP_Gun"}],
  "allow_writes": [],
  "save": false,
  "pie": false,
  "max_retries": 3
}
```

```powershell
python agent/run_task.py --task eval/tasks/bp-gun-compile.json
python agent/run_task.py --query "修掉 TestActor 的编译错误"
python eval/run_eval.py
python eval/report.py data/traces/eval-<timestamp>.jsonl
python eval/check_routes.py
python eval/retrieval_bench.py
```

`report.py` 的列是 id、ok、retry、tools、elapsed、`prompt_tokens`、`completion_tokens`。token 按任务把每行里有的数加起来。没有 usage 就印空白，不估算，也不填 0。最后一行是 `tasks=` 和 `ok` 为 true 的个数。接着按失败行打分类计数：编译、测试、工具拒绝、MCP 连不上、超过重试、关卡预检、其他。对不上就进「其他」。参数如果是 `data/traces` 目录，每个 jsonl 一行：文件名、任务数、`ok` 数、两类 token 合计，然后再打一份合计分类。不算钱，不做网页。`observe` 行带 `judge_kind` 和截短的错误原文，分类靠这两项。`check_routes.py` 通过时打印 `routes ok`，退出码 0。它覆盖「编译通过之后测试失败回到 `fix`」，也覆盖 C++ 换行导出、快照标记、角色上下文、hook 拒绝和分类。不要为了制造失败去改游戏源码。

`--query` 先经 `plan_sequence`。加函数或修编译错误时，写不出唯一的类就失败。编译工程不要求唯一类。拾取的两个槽各自解析，一个槽零条或多条才暂停。蓝图 `refPath` 和 Actor 对不上时不猜。`UE_ALLOW_BLUEPRINT=false` 时，改蓝图的一句话不进图，并在 `Plan/` 写下操作步骤。`true` 时改那张唯一的蓝图，编译失败重试到 `max_retries` 后停。评测仍用 `--task`，避免监督者把三条基线跑歪。`eval/tasks/cpp-fix-keep.json` 与 `cpp-typo` 相同，但 `keep_source: true` 时不还原源码；只有明确要留下修改时才跑。

`run_agent.py` 仍留在磁盘上，菜单不再调用。它是以前的手工对话，没有任务文件时写操作会被拒绝。规划留在 Cursor 的时候不要把它当日常入口，也不要把菜单 3 接成每句话的默认动作。

三条评测，2026-10-12 一次连跑都通过：

| id | 在验什么 | 那次结果 |
| --- | --- | --- |
| `editor-cube` | 把 `L_NewTest` 里名为 Cube 的 Actor 移到 `(1200, 400, 150)`，再读位置。`save: false` | 通过，0 次重试。关卡未保存，重载关卡即回到原位 |
| `bp-gun-compile` | 只编译 `/Game/BP_Gun.BP_Gun` | 通过。不要求改图 |
| `cpp-typo` | 向 `TestActor.cpp` 写入一个不存在的调用，裁判先失败，再 `apply_unified_diff` 删掉它，游戏目标编译通过，最后还原文件 | 通过，1 次重试，trace 里有 `apply_unified_diff` 和 `error C3861` |

`editor-cube` 要求当前关卡名包含 `L_NewTest`。编辑器没开时，这一次调用会先尝试启动编辑器并等待 MCP。关卡名对不上就停，不切换关卡，不保存。`run_eval.py` 在这种情况下把这一条记失败，另外两条继续。

`automation` 裁判已经接上，但默认三条里没有它。`eval/tasks/automation-existing.json` 仍然没有：2026-10-13 编辑器已开，`DiscoverTests` 返回的是 `{"status":"ready"}`，里面没有过滤器。没有能抄的名字就停，不编一个过滤器，也不把这条塞进 `run_eval.py` 的默认三条。

Checkpoint 在 `RAG/ue-rag/agent_checkpoints.db`。每次任务用新的 `thread_id`。

## 仍然没有的东西

这些挡在「任意一句话生成一个会巡逻的敌人」前面，不挡上面三条窄任务，也不挡已经编译过的 `ATestEnemy`：

- 一句话还生成不出这个敌人。C++ 只有巡逻、追击和近战事件。行为树、动画、受击仍由人绑，三个 `BlueprintImplementableEvent` 还没有图
- 蓝图自动改图只在 `UE_ALLOW_BLUEPRINT=true` 时尝试，不保证成功，次数用尽就停。材质、UMG、动画蓝图、Niagara 要各自的 `UE_ALLOW_*` 为 `true` 才放行写工具，缺省仍不改。本轮没有改三个近战事件的图
- 交叉编码器重排、换 Embedding、Docker、评测网页
- 关卡和全量资产仍要关编辑器跑 `asset.bat`。编辑器开着时 `refresh_editor_readonly.py` 可以给已有 Actor 标上快照，并用 `find_assets` 补 `/Game` 里还没有的资产卡片。`find_actors` 只返回 `refPath`，不带回坐标，所以不会把快照数字改成当时的实时位置
- 把 `--query` 设成 Cursor 里每句话的默认动作
- UE Transaction 回滚
- 两个仓库已有 git。关卡和 `Content/` 不入库。可计入成功率的任务在 `RAG/ue-rag/eval/bench_manifest.json`（改关卡且不保存的不进分子分母）。手工 Cursor 对照列还是空的，见 `Plan/20261015-2100-周计划缺口与面试能力升级.md` 第 1 段
- 打包和部署，见 [`Plan/20261013-1545-打包与部署.md`](Plan/20261013-1545-打包与部署.md)
- 索引一致性靠人执行：新建蓝图资产的 run 收尾只提示（事件 `index_stale` + run json `index_hint`），不自动跑 `shortcuts\asset.bat`——它走无界面编辑器，要先关编辑器。L2 归地的 `linked` 分支依赖索引新鲜度，提示没跟上前索引还是旧的
- 理解器只在菜单 3 的多窗口侧生效。菜单 2 单循环仍是纯手写规则，没接 `understand.py`；`A` 类索引里没有的蓝图（例如 `ATestActor` 的 `BP_TestActor`）在 L2 仍会暂停问一次路径——自动创建只在父类可推导时发生，不按类凭空生成实例

## 换机器

改 `RAG/ue-rag/.env`，或依赖 `config.py` 的缺省：

| 变量 | 缺省 |
| --- | --- |
| `UE_MCP_URL` | `http://127.0.0.1:8000/mcp` |
| `UE_EDITOR_CMD` | `D:\UEEngline\UE_5.82\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe` |
| `UE_PROJECT` | `E:\UEProject\AIWork\AIAgentProject\AIAgentProject.uproject` |
| `UE_SOURCE_ROOT` | `E:\UEProject\AIWork\AIAgentProject\Source\AIAgentProject` |
| `UE_EXEC_MODE` | `real` |
| `UE_MCP_TIMEOUT` | `300`（裁判） |
| `UE_MCP_CALL_TIMEOUT` | `60`（普通 MCP） |
| `UE_ALLOW_BLUEPRINT` | `false`。`true` 才允许改蓝图 Graph |
| `UE_ALLOW_MATERIAL` | `false`。`true` 才允许改材质图 |
| `UE_ALLOW_UMG` | `false`。`true` 才允许改 UMG |
| `UE_ALLOW_ANIM` | `false`。`true` 才允许改动画蓝图和 Control Rig |
| `UE_ALLOW_NIAGARA` | `false`。`true` 才允许改 Niagara |
| `UE_LLM_MODEL` | `deepseek-chat`。空着也用这个名字 |
| `DEEPSEEK_API_KEY` | 空。必须自己填 |

`AGENTS.md` 和 `ue-rag-route-skill` 里的 Python 路径仍写着本机 `.venv`。引擎或插件升级后重新跑 `mcp.bat`，否则 `ue_tools` 和编辑器对不上。

不要提交 `.venv`、`data/chroma`、`data/traces`、`.env`。`.env.example` 只列变量名。
