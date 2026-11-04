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
- 让 `ATestActor` 可拾取要同时留下 `bPickable` 和 `InteractableActor`。只写 `Tags` 会和 `ApplyPickable` 分叉。工具目录里没有调用 `BlueprintCallable` 的入口时，用 `set_properties` 写 `bPickable`，用 `add_tag` 加标签，写完再读。读数不对不算成功。写对了按任务策略保存关卡（缺省保存）。
- 新建 Widget Blueprint：`BlueprintTools.create` 的 `asset_type.refPath` 用 `/Script/UMG.UserWidget`，再用 `set_parent` 指到同一类。复查用 `find_assets`，资产类填 `/Script/UMG.WidgetBlueprint`。`save_assets` 保存这一份，关卡按任务策略保存。
- HUD 要「真的会动」就走 C++：新建 `UUserWidget` 子类，四个文本用 `meta=(BindWidgetOptional)` 按名字绑，`WBP_HUD` 用 `set_parent` 指到它再 `compile_blueprint`。`UUserWidget::NativeTick` 要蓝图侧也实现 Tick 才会被驱动（`bHasScriptImplementedTick`），读数改用 `SetTimer` 定时刷新。`APawn` 上没有 `PlayerOwner`（那是 `UUserWidget` 的），挂 HUD 要从 `GetController()` 或 `GetFirstPlayerController()` 取控制器。
- 重叠拾取的有效距离是「球半径 + 胶囊半径」，比 `PickupRadius` 大一圈；再用上 `OnComponentBeginOverlap` 会把相邻金币一起收掉。金币这类「进半径就算」的判定只留每帧距离判定。Pawn 胶囊不要用 `Block` 响应：Block 会盖掉另一侧的 `Overlap`，重叠永不触发；位移走 `AddActorWorldOffset(..., false)` 时胶囊只做 `QueryOnly + Overlap` 就够。
- 资产索引要跟编辑器同步：改关卡或加蓝图后跑 `shortcuts\ingest.bat asset`（关编辑器）。重建会丢掉关卡里已不存在的 Actor 记录，离线检查依赖的目标 Actor（如 `Cube`）必须在关卡里真实存在。`ue_assets.jsonl` 里材质实例的类名是 `MaterialInstanceConstant`，不是 `MaterialInstance`。
- 没见过的需求不要往术语表里加一行短语。分解器只拿「这一句 + 执行器一行卡片 + 最多 3 条检索标题」，模型拆成已有脚本或 `apply_spec`；脚本接不住就丢弃，不改写成别的动作。`AFpsPawn` 不自动占据玩家，不改 `ATestGameMode`。
- 临时世界里的 Automation（`CreateWorld` 再 Spawn，测完销毁）通过，不能当成句子点名的关卡里已经有玩法。先加载那个关卡，再按类计数核对；缺了人或靶、地面、灯，失败数 0 也不算通过。
- 射击关要能开枪，就把这一关的 GameMode Override 设为 `FpsGameMode`。不改全局 `BP_GameMode`，不改 `ATestGameMode`，也不把 `AFpsPawn` 改成自动占据。
- monolith 插件自带完整 Niagara 动作集（`niagara.create_system` 模板用 `/Niagara/DefaultAssets/DefaultSystem`、`add_module`/`set_module_input_value`/`set_module_enabled`/`request_compile`、验编译用 `get_system_diagnostics`），不用切 official 后端；模块脚本资产在 `/Niagara/Modules/...`，Burst 类在 `Emitter/` 子目录。
- 新增 UCLASS 必须关编辑器全量重建，Live Coding 干不了；Live Coding 激活时 `Build.bat` 直接报 OtherCompilationError。Niagara 公开头 include 不带 `Niagara/` 前缀；`TSoftObjectPtr` 从路径字符串构造要走 `FSoftObjectPath`。
- 非交互跑 start.bat：chcp 65001 在 stdin 被重定向时会丢掉已缓冲输入（入口读到 EOF 空转）。已改为 UE_RAG_STDIN 定义时跳过 chcp，:loop 分支同样吃 UE_RAG_STDIN，喂「需求行 + 兜底裁决行」的文件即可。裁决行若没被暂停消费，会被 REPL 当成下一条需求——非交互只喂确定会暂停的裁决，跑完确认退出码。/Game 裸根目录不算合法补路径，要用 /Game/目录 或 /Game/BP_名。
- 模型网关（newapi.opensosen.com）的模型名会下架：deepseek-flash 已无可用渠道（503 model_not_found），现用 deepseek-v4.1-flash；网关对 urllib 默认 UA 返回 Cloudflare 403/1010，探活要带 UA。Windows User/Machine 级环境变量里的旧 DEEPSEEK_API_KEY 会压过 .env（load_dotenv 默认不覆盖），表现为理解器 401「无效的令牌」；config._load_env 已改 override=True 以 .env 为准，UE_LLM_MODEL 已改 deepseek-v4.1-flash。
- FPS 关「视角/射击/手雷全失灵但绑定代码都在」：先查 Possess 被谁抢——世界子系统 OnWorldBeginPlay 里生成替身并 Possess 是隐形劫持高发点（SimpleVehicleWorldSubsystem 教训）。这类自动劫持要用 cvar 门（simplevehicle.AutoSpawn，默认 0），不要无条件抢玩家控制器。多窗口并行 solve 各写一套同名类，编译债会拖到下一次 Build 才以 UHT shares engine name 爆雷：孤儿副本删前先确认零外部引用（include 与类名两路 grep）。
- monolith 后端关编辑器：先 list_dirty_packages 确认 0 脏包，再 run_console_command QUIT_EDITOR；重编命令是 Build.bat AIAgentProjectEditor Win64 Development -Project=<uproject> -WaitMutex（judge.cpp_build_argv 同款）。
- 载具自由视角：车身不设 bUseControllerRotationYaw，相机 bUsePawnControlRotation=true，鼠标 BindAxisKey(MouseX/MouseY) 只喂 AddControllerYaw/PitchInput——视角与转向解耦，车身朝向仍由 A/D 驱动。上下车走「走路 Pawn 绑 E 找最近载具 → SetDriver+Possess(载具)；载具绑 E → ExitVehicle → Possess 回原 Pawn」；SetDriver 收 APawn* 别限 ACharacter*（FpsPawn 不是 Character）。放进关卡的 native Actor 重编后自动长出新增默认子对象，不用重放。
- APawn 子类的局部变量别叫 Controller（遮蔽 APawn::Controller，C4458 当错误）。编辑器内只读探查脚本输出超 200 字会被 monolith_backend.py 截断——让 python 写临时文件再读。
- 后台/超时方式调 start.bat 的调用方要 kill 整棵 cmd 进程树：残留的 cmd 会拿着旧 stdin 文件在 REPL 里自己起新 run（20261028-192951 孤儿 run：静默改了源码、编了译，没写 run md 没归档——无主写入者比失败更危险，证据链断在半空）。
- 离线检查的「不连编辑器/不依赖 .env」必须用代码钉住：eval/_pin_offline.close_now() 进程级封闭（本机 .env 开关不渗进断言），走 exec_editor_tool 的断言一律配假 exec_tool；agent 里裸 `load_dotenv()` 是禁手（按 CWD 自己找 .env，会把封闭打穿），统一 `load_dotenv(config.ENV_PATH, override=True)`。
- solver（agent/solver.py）的诚实口径三条底线：改需求的句子方案里必须有交付性改动（patch/apply_spec/写原语；开关卡/保存/编译/观察不算，见 _SOLVER_INFRA_ACTIONS）；有写码步骤必须真编译（build_rounds>0）；automation 收口绿只对纯只读任务算完成。参数形状不靠猜：describe 校准（_calibrate_args）。
