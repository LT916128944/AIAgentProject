# 心跳配置迁移到 DeveloperSettings 实施计划

## 1. Summary

把心跳上报配置从 `UMeetVRHeartbeatSubsystem` 内的 `FMeetVRHeartbeatConfig` 结构体，迁移到专用的 **`UMeetVRHeartbeatDeveloperSettings`（继承 `UDeveloperSettings`）**，使其可通过 **Project Settings 面板**（MeetVR 分类）与 **ini 文件**（`Config/DefaultMeetVRTemplate.ini`）配置，运行期由子系统经 `Get()` 读取。

沿用同模块既有模板 `UMeetVRLakeItemDeveloperSettings`（`UCLASS(config = MeetVRTemplate, defaultconfig, ...)`），头文件-only、最小侵入。

## 2. Current State Analysis

### 当前心跳配置（本次会话已实现）
- [MeetVRHeartbeatSubsystem.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/GameModes/MVL_Backend/MeetVRHeartbeatSubsystem.h)：`FMeetVRHeartbeatConfig { float HeartbeatIntervalSeconds = 10.0f; }`（BlueprintType USTRUCT），子系统持有 `CurrentConfig` 成员，`StartHeartbeat(UObject*, const FMeetVRHeartbeatConfig&)` / `StartHeartbeatDefault(UObject*)` 手动启动。
- [MeetVRHeartbeatSubsystem.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/GameModes/MVL_Backend/MeetVRHeartbeatSubsystem.cpp)：`Initialize()` 自动 `StartHeartbeat(this, FMeetVRHeartbeatConfig())`；`StartHeartbeat` 内 `FMath::Clamp(Config.HeartbeatIntervalSeconds, 1.0f, 60.0f)` 后 `SetTimer`。

### 参考模板（同模块既有 DeveloperSettings）
- `Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/MeetVRLakeItemDeveloperSettings.h`：
  - `UCLASS(config = MeetVRTemplate, defaultconfig, DisplayName = "MeetVR LakeItem")`
  - `static const ...* Get() { return GetDefault<...>(); }`
  - `virtual FName GetCategoryName() const override { return FName("MeetVR"); }`
  - 字段 `UPROPERTY(config, EditAnywhere, Category = "...", meta = (DisplayName = "..."))`
  - 纯头文件（Get/GetCategoryName 均 inline），无 cpp。
  - 对应 ini 段：`Config/DefaultMeetVRTemplate.ini` 中 `[/Script/MeetVRTemplate.MeetVRLakeItemDeveloperSettings]`。

### 消费面确认
- `FMeetVRHeartbeatConfig` / `StartHeartbeat` / `StartHeartbeatDefault` **仅有子系统内部使用**，无任何外部 C++ 消费者（已全局检索），可安全重构。

### 依赖
- `UDeveloperSettings` 位于 Engine 模块，`MeetVRTemplate.Build.cs` 已依赖 `DeveloperSettings`（Private），**无需改 Build.cs**。

## 3. Proposed Changes

### 3.1 新建头文件
`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/GameModes/MVL_Backend/MeetVRHeartbeatDeveloperSettings.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MeetVRHeartbeatDeveloperSettings.generated.h"

/**
 * 心跳上报专用 DeveloperSettings。
 * Project Settings 面板 → MeetVR 分类下可见。
 * ini 段：[/Script/MeetVRTemplate.MeetVRHeartbeatDeveloperSettings]
 * UMeetVRHeartbeatSubsystem 在运行时通过 Get() 读取，配置从 Subsystem 字段解耦。
 */
UCLASS(config = MeetVRTemplate, defaultconfig, DisplayName = "MeetVR Heartbeat")
class MEETVRTEMPLATE_API UMeetVRHeartbeatDeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 便捷读取入口（const 版本，仅读场景） */
    static const UMeetVRHeartbeatDeveloperSettings* Get() { return GetDefault<UMeetVRHeartbeatDeveloperSettings>(); }

    //~UDeveloperSettings interface
    virtual FName GetCategoryName() const override { return FName("MeetVR"); }
    //~End of UDeveloperSettings interface

    // ===== Heartbeat =====

    /** 心跳上报间隔（秒）。文档建议 5–10s，默认 10s（服务端在线阈值 30s） */
    UPROPERTY(config, EditAnywhere, Category = "Heartbeat",
              meta = (DisplayName = "Heartbeat Interval Seconds", ClampMin = "1.0", ClampMax = "60.0"))
    float HeartbeatIntervalSeconds = 10.0f;
};
```

### 3.2 修改 `MeetVRHeartbeatSubsystem.h`
- **删除** `FMeetVRHeartbeatConfig` 结构体（配置迁往 DeveloperSettings）。
- `StartHeartbeat` 签名改为 `void StartHeartbeat(UObject* WorldContextObject);`（去掉 Config 参数）。
- **删除** `StartHeartbeatDefault`（原仅用于传默认配置，现冗余）。
- **删除** `CurrentConfig` 成员。
- 其余（`StopHeartbeat` / `IsHeartbeating` / `HeartbeatOnce` / `TryAutoInitialize` / `SendHeartbeat`）不变。头文件无需 include DeveloperSettings（仅 cpp 使用）。

### 3.3 修改 `MeetVRHeartbeatSubsystem.cpp`
- 顶部新增 include：`#include "GameModes/MVL_Backend/MeetVRHeartbeatDeveloperSettings.h"`。
- `Initialize()`：`StartHeartbeat(this);`（不再传默认 Config）。
- `StartHeartbeat(UObject* WorldContextObject)` 重写间隔来源：

```cpp
void UMeetVRHeartbeatSubsystem::StartHeartbeat(UObject* WorldContextObject)
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Heartbeat] StartHeartbeat failed: GameInstance is null"));
        return;
    }

    const UMeetVRHeartbeatDeveloperSettings* Settings = UMeetVRHeartbeatDeveloperSettings::Get();
    const float Interval = FMath::Clamp(Settings ? Settings->HeartbeatIntervalSeconds : 10.0f, 1.0f, 60.0f);
    GI->GetTimerManager().SetTimer(
        HeartbeatTimerHandle, this, &UMeetVRHeartbeatSubsystem::SendHeartbeat, Interval, true);

    bIsHeartbeating = true;
    UE_LOG(LogTemp, Log, TEXT("[Heartbeat] Periodic heartbeat started, interval=%.2fs"), Interval);

    // 首次立即发送一次，不必等待一个周期
    SendHeartbeat();
}
```

- **删除** `StartHeartbeatDefault` 实现。
- 其余逻辑不变。

### 3.4 修改 `Config/DefaultMeetVRTemplate.ini`
在文件末尾追加（与 LakeItem 段并列，默认值 10s 便于面板/文档可见）：

```ini
[/Script/MeetVRTemplate.MeetVRHeartbeatDeveloperSettings]
HeartbeatIntervalSeconds=10.0
```

### 3.5 不改动项
- 不改 `MpyApiClient` 生成代码、`MeetVRChapterReportSubsystem`、`MeetVRNetworkMetricSubsystem`、`MeetVRTemplate.Build.cs`、`MeetVRTemplate.uplugin`。

## 4. Assumptions & Decisions

| 决策点 | 决定 | 理由 |
| --- | --- | --- |
| 承载类 | 独立 `UMeetVRHeartbeatDeveloperSettings : UDeveloperSettings` | 用户明确要求走 DeveloperSettings，可从项目配置 |
| 模板 | 仿 `UMeetVRLakeItemDeveloperSettings`（`config=MeetVRTemplate`、头文件-only） | 同模块既有范式，ini 段自动落 `[/Script/MeetVRTemplate.*]` |
| 配置项 | 仅 `HeartbeatIntervalSeconds`（默认 10s，clamp 1–60s） | 与现有配置一致，不多不少 |
| `FMeetVRHeartbeatConfig` 去留 | 删除（连同 `CurrentConfig` 成员） | 单一数据源；结构体无外部消费者，删除避免双源不一致 |
| `StartHeartbeatDefault` 去留 | 删除 | 仅为传默认配置的别名，配置迁出后冗余 |
| `StartHeartbeat` 签名 | `StartHeartbeat(UObject* WorldContextObject)` | 保留 BP 世界上下文约定，与 `MeetVRNetworkMetricSubsystem::StartReporting` 一致 |
| 空指针兜底 | `Settings ? Settings->HeartbeatIntervalSeconds : 10.0f` | DeveloperSettings 加载异常时回退默认值，不崩溃 |
| ini 段 | `[/Script/MeetVRTemplate.MeetVRHeartbeatDeveloperSettings]` | `config = MeetVRTemplate` 自动生成该段 |
| 验证范围 | 不做编译验证（沿用用户「暂不考虑编译验证」要求），仅 IDE 诊断 + 运行时验证 | 与上一计划保持一致 |

## 5. Verification

1. **IDE 诊断**：`GetDiagnostics` 检查 `MeetVRHeartbeatDeveloperSettings.h` 与修改后的 `MeetVRHeartbeatSubsystem.h/.cpp` 无错误。
2. **Project Settings 可见性**（编译后）：UE 编辑器 → Project Settings → Plugins → MeetVR 分类 → 出现 "MeetVR Heartbeat" 配置页，可修改「Heartbeat Interval Seconds」。
3. **ini 持久化**：修改面板值并保存 → `Config/DefaultMeetVRTemplate.ini` 的 `[/Script/MeetVRTemplate.MeetVRHeartbeatDeveloperSettings]` 同步更新。
4. **运行时生效**：启动后日志 `[Heartbeat] Periodic heartbeat started, interval=X.XXs` 与面板配置一致；将间隔改为 5s 后日志显示 5.00s。
5. **默认回退**：ini 未配置时使用 C++ 默认 10s（日志确认）。
