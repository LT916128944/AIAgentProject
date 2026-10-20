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
- Python Toolset 的 `Registration` 在 `toolset_registry.registration`，不在 `toolset_registry` 包根上。`@toolset_registry.tool_call` 才从包根导出。
- AgentSkill 用 `ToolsetRegistry.AgentSkillToolset.CreateSkill` 建在 `/Game` 下。`GetSkills` 返回的 instructions 是 `GeneratePrompt` 的结果。保存时用资产路径，不带 `_C`。
- Actor 没有可移动的场景根时，`SetActorLocation` 不改变 `GetActorLocation`。要巡逻位移就把根做成 `USceneComponent` 且 `Movable`。`BlueprintImplementableEvent` 的次数桩写在调用前，事件本身仍留给蓝图。
- Material Instance 覆写参数的前提是父材质已定义该参数：`set_scalar_parameter` 对不存在的参数名直接拒绝。引擎 MCP 没有任何工具能建 Material 资产（`BlueprintTools.create` 对 Material 报错且回包断连）；建法是项目 python Toolset（`Content/Python/aiagent_material_toolset.py`，MaterialFactoryNew），注册名 = 模块名.类名，编辑器重启才加载。
- 自动化测试的临时世界（CreateWorld + BeginPlay 后再 SpawnActor）不派发 BeginPlay，也不自动 Tick 组件：测试显式驱动启动入口（StartXxx/BindTo），组件用 `->TickComponent(dt, LEVELTICK_All, nullptr)`，Actor 位置逻辑用公有 `Tick`。
- `ue_mcp_client` 的返回 `data` 已是 dict（returnValue 在其中），不要再 json.loads 一遍字符串假设。UE 5.8 overlap 读数用 `GetGenerateOverlapEvents()`。
- 键盘字母不是 Axis1D。`BindAxisKey` 只能绑鼠标轴、手柄轴；WASD 用 `BindKey` 的 Pressed / Released。`DefaultInput.ini` 里的 `MoveForward` / `MoveRight` 还要 `BindAxis`。两路相加后夹到 1，避免只通一路时不动，也避免两路都通时速度翻倍。场景根是 `USceneComponent` 时，`AddActorWorldOffset` 的扫掠用 false。鼠标看向用 `BindAxisKey(MouseX/MouseY)`，不要绑 ini 里不存在的 `Turn` / `LookUp`。`MouseY` 取反后再交给 pitch。
- 本引擎 `UInputComponent::BindKey` 只有 4 参数成员函数指针重载，不接受 lambda。要区分按键时用 `void(FKey)`。
- C++ 导出按配平后的 `UCLASS(...)` 认类。`UCLASS` 和 `class 模块_API 类名` 之间可以换行，参数里的括号要配平，同一文件里的多个类都会导出。Actor 坐标是导出当时的快照，不是实时位置。
- 让 `ATestActor` 可拾取要同时留下 `bPickable` 和 `InteractableActor`。只写 `Tags` 会和 `ApplyPickable` 分叉。工具目录里没有调用 `BlueprintCallable` 的入口时，用 `set_properties` 写 `bPickable`，用 `add_tag` 加标签，写完再读。读数不对不算成功。不保存关卡。
