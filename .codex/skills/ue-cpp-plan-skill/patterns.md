# UE C++ 分层默认

默认，不是教条。项目里已有相反约定时，跟项目。

## 放哪

| 职责 | 默认类型 | 不要 |
|------|----------|------|
| 跨关卡数据 / HTTP / 上报 | `UGameInstanceSubsystem` | 把配置字段堆在 Subsystem 上 |
| 项目级可配项（URL、间隔、开关） | `UDeveloperSettings`（`GetDefault` / `Get()`） | 运行时可变业务状态放 Settings |
| 纯计算、可挂多个实例 | `UActorComponent` | 把全局存储放进 Component |
| 关卡批量生成 | `AActor` LevelManager（可拖入关卡） | Manager 里写权重/尺寸等计算 |
| 触发入口 | 可挂任意 Actor 的 `USceneComponent` | 假设用户操作 Actor 的基类 |
| 端点解耦 | `UInterface` | 硬绑具体玩法类 |

## 职责切分

- **不写巨型类**：一个 Actor 只留生成、挂载和已有钩子。血量、背包、AI 放 `UActorComponent`、`UGameInstanceSubsystem` 或 `AAIController`。单文件短到一次 diff 能打中。
- **数据层完整**：C++ 做查询、生命周期、网络/文件 IO。视觉与运动表现用 `BlueprintImplementableEvent` 或 `BlueprintNativeEvent`，图由人接。
- **单一职责**：存储/下载在 Subsystem；抽取/计算在 Component；生成在 Manager；触发在 Marker。
- **配置与运行时解耦**：ini / Project Settings 走 DeveloperSettings；Subsystem 运行时 `Get()` 读取。
- **最小类型**：DataTable 行结构只留查询/抽取必需字段；寿命、速度、去向留给对象自己。

## 落地习惯

- 目录：`Public/<Feature>/` + `Private/<Feature>/`，与模块内已有功能文件夹对齐
- 命名：跟项目前缀（不要另起一套）
- 头文件：前向声明；具体 include 放 `.cpp`
- 指针：头文件 `TObjectPtr<>`；类引用 `TSubclassOf<>`
- 依赖：先确认 `Build.cs` 已有模块，没有再加；能不改就不改
