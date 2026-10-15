# MVL_Backend 按启动参数（-Role / -ViewMode）配置功能 — 实施计划（v3）

> 状态：规划中。上游（MeetVRLBE/MVLAppCore）已完成参数解析；本计划仅剩 MVL_Backend 的功能开关实施。
> 范围：`Plugins/MeetVRTemplate/Source/MeetVRTemplate/GameModes/MVL_Backend`
> 约束（用户确认）：**不新增 MeetVRRoleConfig 模块**，在各门控点直接读取角色并判断。

---

## 1. 需求表格

| 功能 | DS 服务器 | 客户端 | 单机 |
|---|---|---|---|
| 心跳 | 1 | 1 | 1 |
| 汇报 ready | 1 | | |
| 等待开始 | 1 | | 1 |
| 登录 | 1 | | |
| 汇报位置 | 1 | | |
| 同步位置 | 1 | | |
| 埋点 | 1 | | 1 |
| 网络指标 | 1 | | |

---

## 2. 现状（已核实）

### 2.1 上游已完成（MeetVRLBE / MVLAppCore，无需再改）

- `FMVLLaunchParams`（`MVLAppCore.h`）新增：`Role`（命令行 `-role=`，小写）、`BaseURL`（替代原 `Host`）；已在 `MVLAppCore.cpp` 解析。
- `UMVLAppGameInstanceSubsystem` 新增：`RoleKey="role"`、`ViewModeKey="viewMode"`、`RoleValue`、`ViewModeValue`，`Initialize()` 里 `GetLaunchArgs` 读取（Android=Intent，PC=命令行）；getter `GetMVLRoleValue()` / `GetMVLViewModeValue()`。
- MVL_Backend 5 处 `Params.Host` → `Params.BaseURL` 已同步改名。

### 2.2 MVL_Backend 尚未实施

| 文件 | 当前门控（旧逻辑） | 目标 |
|---|---|---|
| `MeetVR_MVLGameModeComponent.cpp` L58 | `if (Params.IsDSMode())` | 仅 DS（ready/登录） |
| `MeetVR_MVLGameFlowComponent.cpp` L46 | `if (CachedTeamId > 0 && !HostUrl.IsEmpty())` | DS + 单机（等待开始） |
| `MeetVR_MVLGameTeamComponent.cpp` L36 | 同上 | 仅 DS（位置 GET/POST） |
| `MeetVR_MetricReportSubsystem.cpp` | 无门控（客户端也会发埋点） | DS + 单机 |
| `MeetVRNetworkMetricSubsystem.cpp` L33-41 | `IsDedicatedServerInstance()` | 仅 DS |

---

## 3. 方案设计（无 MeetVRRoleConfig，直接门控）

### 3.1 角色来源与统一判断片段（不新建模块）

角色来源（用户确认分工）：
- **DS**：`FMVLAppCoreModule::GetLaunchParams().Role`（命令行 `-role=ds`）
- **Client / Standalone**：`UMVLAppGameInstanceSubsystem::GetMVLRoleValue()`（Android Intent `role` / PC `-role=`）
- **兜底**：`IsDSMode() ? ds : client`

为减少重复，每个需要判断的 .cpp 文件内放一段**文件级 static 辅助函数**（约 10 行，不新增模块、不跨文件共享）：

```cpp
// 获取生效角色字符串：1) 命令行 -role=（DS）；2) AppSubsystem role（client/standalone）；3) 兜底
static FString ResolveMVLRole(const UObject* WorldContext)
{
    const auto& Params = FMVLAppCoreModule::GetLaunchParams();
    if (!Params.Role.IsEmpty()) { return Params.Role; }
    if (UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr)
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UMVLAppGameInstanceSubsystem* App = GI->GetSubsystem<UMVLAppGameInstanceSubsystem>())
            {
                if (!App->GetMVLRoleValue().IsEmpty()) { return App->GetMVLRoleValue(); }
            }
        }
    }
    return Params.IsDSMode() ? TEXT("ds") : TEXT("client");
}

// 仅判断是否 DS（DS 全来自命令行，无需 WorldContext）
static bool IsMVLDS()
{
    const auto& Params = FMVLAppCoreModule::GetLaunchParams();
    return Params.IsDSMode() || Params.Role.Equals(TEXT("ds"));
}
```

字符串统一小写比较（`ds / client / standalone`）；`-role=` 由 `FCString::Strfind` 大小写不敏感匹配，`-Role=` 大写也可。

### 3.2 逐功能改造

| # | 文件 | 改动 |
|---|---|---|
| 1 | `MeetVR_MVLGameModeComponent.cpp` L58 | `if (Params.IsDSMode())` → `if (IsMVLDS())`（本文件加 `IsMVLDS`）。ready/登录仅 DS |
| 2 | `MeetVR_MVLGameFlowComponent.cpp` L46 | 条件改为 `const FString Role = ResolveMVLRole(this); if ((Role.Equals(TEXT("ds")) \|\| Role.Equals(TEXT("standalone"))) && CachedTeamId > 0 && !HostUrl.IsEmpty())`（本文件加 `ResolveMVLRole`）。等待开始 DS+单机 |
| 3 | `MeetVR_MVLGameTeamComponent.cpp` L36 | 条件改为 `if (IsMVLDS() && CachedTeamId > 0 && !HostUrl.IsEmpty())`（本文件加 `IsMVLDS`）。位置 GET/POST 仅 DS |
| 4 | `MeetVR_MetricReportSubsystem.cpp` | `TryAutoInitialize()` / `PostMetric()` 入口加：`const FString Role = ResolveMVLRole(this); if (Role.Equals(TEXT("client"))) return;`（本文件加 `ResolveMVLRole`）。埋点 DS+单机 |
| 5 | `MeetVRNetworkMetricSubsystem.cpp::ShouldCreateSubsystem` L33-41 | 改为 `const auto& Params = FMVLAppCoreModule::GetLaunchParams(); return Params.IsDSMode() \|\| Params.Role.Equals(TEXT("ds"));`（DS 全来自命令行，直接判断） |
| 6 | `MeetVRHeartbeatSubsystem.cpp` | **不改**（三端全开）。可选：单机心跳 actorType 按需加 "standalone" 分支 |
| 7 | `MeetVR_MVLServerSessionSubsystem.cpp` | **不改**：`InitializeDS` 仅 DS 被调用 → ready/登录/重连自然仅 DS |
| 8 | `MeetVRChapterReportSubsystem.cpp` | **不改**：`TryAutoInitialize` 已按 IsDSMode 跳过非 DS |

说明：
- #1/#3/#5 只判断 DS，角色全来自命令行（`-role=ds` 或 `IsDSMode()` 兜底），无需 AppSubsystem，逻辑最简单。
- #2/#4 需要区分 client/standalone，用 `ResolveMVLRole(this)` 取合并后的角色字符串。
- 不引入任何新文件、不新增枚举；每个文件仅多一段文件级 static 函数。

### 3.3 ViewMode

- 只做：读取 + 打日志 + 如需 Blueprint 用 `AppSubsystem->GetMVLViewModeValue()`（已暴露）。
- 本计划不接业务逻辑；日志可加在需要处（如 GameFlow/GameMode 的 OnInitGame 打一条 `[MVL] Role=xxx, ViewMode=xxx`）。

---

## 4. 假设与决策

1. **不使用 MeetVRRoleConfig**（用户确认）：门控点直接读 `FMVLLaunchParams.Role`（DS）/ `AppSubsystem.GetMVLRoleValue()`（client/standalone）。
2. **解析优先级**：`Params.Role` → `GetMVLRoleValue()` → `IsDSMode() ? ds : client`。
3. **字符串比较**：小写 `ds / client / standalone`；`-Role=`（大写）也能被解析匹配。
4. **单机（Standalone）**：平台不确定，role 从 AppSubsystem 拿（Android Intent 或 PC `-role=`）。`等待开始`保留 `CachedTeamId > 0` 守卫。
5. **埋点门控**：发送前拦截（`TryAutoInitialize`/`PostMetric`），不用 `ShouldCreateSubsystem`，避免 Android role 在子系统创建期未就绪误判。
6. **网络指标**：DS 全来自命令行，`ShouldCreateSubsystem` 直接判断 cmdline 即可。
7. **心跳**：三端全开不改；单机 actorType 暂按 client。
8. 不动任何 Blueprint 资产（GM_MeetVR 三端共用）。

---

## 5. 实施顺序

1. 改 `MeetVR_MVLGameModeComponent.cpp`（IsMVLDS + 门控）。
2. 改 `MeetVR_MVLGameFlowComponent.cpp`（ResolveMVLRole + 门控）。
3. 改 `MeetVR_MVLGameTeamComponent.cpp`（IsMVLDS + 门控）。
4. 改 `MeetVR_MetricReportSubsystem.cpp`（ResolveMVLRole + 拦截）。
5. 改 `MeetVRNetworkMetricSubsystem.cpp`（ShouldCreateSubsystem 用 cmdline role）。
6. 编译验证（Win64 Editor / Server）。

## 6. 验证

- **DS**（`-role=ds -teamId=X -dsToken=Y -baseUrl=H`）：日志出现 8 项对应行为。
- **客户端（Android）**：Intent 传 `role=client`，日志仅见心跳；埋点/网络指标/位置轮询不触发。
- **单机（Standalone）**：Intent `role=standalone`（Android）或 `-role=standalone`（PC）：心跳 + 等待开始（若带 teamId）+ 埋点生效；ready/登录/位置/网络指标不触发。
- 不带 role 的旧启动方式行为不变（回退逻辑）。
