# 跨天约定

写 Plan 或任务记录前只读本文件。不要扫 `plans/`、`任务记录/` 的其他日期。

## 怎么做

- 先仿后造：锁定 1 个同模块范式。签名 + 职责即可，不贴整份头文件。
- 玩法拆开：血量、背包、AI 放 Component、Subsystem 或 Controller。Actor 只留生成、挂载和已有钩子。表现用 `BlueprintImplementableEvent`，图由人接。
- 配置进 `UDeveloperSettings`（`Get()` / `GetDefault`）。运行时状态不要堆在 Settings 上。分层细节见 `ue-cpp-plan-skill/patterns.md`。
- 能不改 `Build.cs` 就不改。

## 经验升级

- 任务记录里的「经验」下次还会用 → 追加一条到本文件。
- 一次性坑留在当天 `任务记录/YYYY-MM-DD.md`，不写进来。
- 本文件不收其他工程的命名、枚举或玩法约定。

## 已沉淀

- 长度：1 米 = 100uu。速度用 uu/s，1 米/秒 = 100。
- 键盘字母不是 Axis1D。`BindAxisKey` 只能绑鼠标轴、手柄轴；WASD 用 `BindKey` 的 Pressed / Released。`DefaultInput.ini` 里的 `MoveForward` / `MoveRight` 还要 `BindAxis`。两路相加后夹到 1，避免只通一路时不动，也避免两路都通时速度翻倍。场景根是 `USceneComponent` 时，`AddActorWorldOffset` 的扫掠用 false。鼠标看向用 `BindAxisKey(MouseX/MouseY)`，不要绑 ini 里不存在的 `Turn` / `LookUp`。`MouseY` 取反后再交给 pitch。
- 本引擎 `UInputComponent::BindKey` 只有 4 参数成员函数指针重载，不接受 lambda。要区分按键时用 `void(FKey)`。
- C++ 导出按配平后的 `UCLASS(...)` 认类。`UCLASS` 和 `class 模块_API 类名` 之间可以换行，参数里的括号要配平，同一文件里的多个类都会导出。Actor 坐标是导出当时的快照，不是实时位置。
- 让 `ATestActor` 可拾取要同时留下 `bPickable` 和 `InteractableActor`。只写 `Tags` 会和 `ApplyPickable` 分叉。工具目录里没有调用 `BlueprintCallable` 的入口时，用 `set_properties` 写 `bPickable`，用 `add_tag` 加标签，写完再读。读数不对不算成功。不保存关卡。
