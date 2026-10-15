# MeetVRDanmaku DeveloperSettings 解耦计划

## 一、Summary / 摘要

新增专用项目级配置对象 `UMeetVRDanmakuDeveloperSettings`（继承 `UDeveloperSettings`），把 `UMeetVRDanmakuDatabaseSubsystem` 中 3 个失效的 `EditDefaultsOnly` 配置字段（用户已移除 `Config` meta，导致 ini 不再加载）解耦到独立 DeveloperSettings 类，恢复 ini 持久化 + Project Settings 面板可视化编辑。

设计原则：**最小侵入**，仅修改 DatabaseSubsystem 与新增一个 DeveloperSettings 文件；不动 SpawnComponent/Actor/LevelManager 等其他类。

## 二、Current State Analysis / 现状分析

### 失效的配置链路

| 来源 | 状态 |
|------|------|
| `DefaultMeetVRDanmaku.ini` 段 `[/Script/MeetVRDanmaku.MeetVRDanmakuDatabaseSubsystem]` | 存在但**已失效**（用户已移除字段 Config meta，UE 不再从 ini 读取） |
| `UMeetVRDanmakuDatabaseSubsystem.h` 的 3 个 `protected` 字段 | `UPROPERTY(EditDefaultsOnly, ...)` 无 `config` 标记，无法持久化 |
| 3 个字段在 cpp 中的使用点 | `RequestDanmakuBatchFromDefaultURL` 读 `DefaultServerURL`；`GetNext` 读 `bCyclePool`；`HandleRequestComplete` 读 `bClearBeforeBatch` |

### 项目参考实现（Phase 1 探索）

- `Plugins/VirtualRealityLocationBasedSystem/Source/ExperienceZone/Public/ExperienceZoneSettings.h`：**最简模板**
  - `UCLASS(config = ExperienceZoneSettings, defaultconfig, DisplayName = "...")`
  - 继承 `UDeveloperSettings`
  - `static UExperienceZoneSettings* Get() { return GetMutableDefault<UExperienceZoneSettings>(); }`
  - 字段 `UPROPERTY(config, EditAnywhere, Category = "...")`
- `Plugins/MeetVRTemplate/.../MeetVRDeveloperSettings.h`：复杂版（`UDeveloperSettingsBackedByCVars`，CVar 联动）—— 不采用，弹幕系统不需要 CVar

采用 ExperienceZoneSettings 模板（简洁、清晰、单类）。

### 决策依据（用户回答）

- **解耦范围**：仅数据库 3 项（DefaultServerURL/bCyclePool/bClearBeforeBatch）
- **类结构**：单类 `UMeetVRDanmakuDeveloperSettings`，按 Category 分组（Database|Download、Database|Cursor）

## 三、Proposed Changes / 提案变更

### 新增文件

#### `Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Public/MeetVRDanmakuDeveloperSettings.h`

**职责**：弹幕系统项目级配置对象，集中数据库层 3 个配置项，支持 ini 持久化 + Project Settings 面板编辑。

**关键代码**：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MeetVRDanmakuDeveloperSettings.generated.h"

/**
 * 弹幕系统专用 DeveloperSettings。
 * Project Settings 面板 → MeetVR 分类下可见。
 * ini 段：[/Script/MeetVRDanmaku.MeetVRDanmakuDeveloperSettings]
 * UMeetVRDanmakuDatabaseSubsystem 在运行时通过 Get() 读取，把数据库配置从 Subsystem 字段解耦。
 */
UCLASS(config = MeetVRDanmaku, defaultconfig, DisplayName = "MeetVR Danmaku")
class MEETVRDANMAKU_API UMeetVRDanmakuDeveloperSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** 便捷读取入口（const 版本，仅读场景） */
    static const UMeetVRDanmakuDeveloperSettings* Get() { return GetDefault<UMeetVRDanmakuDeveloperSettings>(); }

    //~UDeveloperSettings interface
    virtual FName GetCategoryName() const override { return FName("MeetVR"); }
    //~End of UDeveloperSettings interface

    // ===== Database | Download =====

    /** 默认弹幕数据服务器 URL（RequestDanmakuBatchFromDefaultURL 使用） */
    UPROPERTY(config, EditAnywhere, Category = "Database|Download", meta = (DisplayName = "Default Server URL"))
    FString DefaultServerURL = TEXT("http://localhost:8080/danmaku");

    /** HTTP 下载批量前是否先 Clear 数据库 */
    UPROPERTY(config, EditAnywhere, Category = "Database|Download", meta = (DisplayName = "Clear Before Batch"))
    bool bClearBeforeBatch = false;

    // ===== Database | Cursor =====

    /** 取下一条后是否循环复用游标（true=循环；false=取出后从库移除） */
    UPROPERTY(config, EditAnywhere, Category = "Database|Cursor", meta = (DisplayName = "Cycle Pool"))
    bool bCyclePool = true;
};
```

**设计要点**：
- `UCLASS(config = MeetVRDanmaku, defaultconfig, ...)` → ini 配置段名 `MeetVRDanmaku`，UE 自动写到 `[/Script/MeetVRDanmaku.MeetVRDanmakuDeveloperSettings]`
- 字段用 `config` meta → 保证 ini 持久化（区别于 DatabaseSubsystem 现在的 `EditDefaultsOnly` 不带 `config`）
- `Get()` 返回 `const` 版本（数据库只读取不修改配置）
- 重写 `GetCategoryName()` 返回 `"MeetVR"`，在 Project Settings 面板的 MeetVR 分类下显示
- 默认值与原 DatabaseSubsystem 字段默认值一致（URL=`http://localhost:8080/danmaku`，bCyclePool=true，bClearBeforeBatch=false）
- 无 cpp 文件（Get() 和 GetCategoryName() 都是 inline）

### 修改文件

#### `Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Public/MeetVRDanmakuDatabaseSubsystem.h`

**改动**：删除 `protected` 区段中 3 个字段（DefaultServerURL/bCyclePool/bClearBeforeBatch）。

**改动前（用户当前版本）**：

```cpp
protected:
    /** 默认服务器 URL（可被 ini 覆盖） */
    UPROPERTY(EditDefaultsOnly, Category = "MeetVR|Danmaku|Download")
    FString DefaultServerURL;

    /** 取完后循环复用而非清空 */
    UPROPERTY(EditDefaultsOnly, Category = "MeetVR|Danmaku|Database")
    bool bCyclePool = true;

    /** 下载批量前是否清库 */
    UPROPERTY(EditDefaultsOnly, Category = "MeetVR|Danmaku|Download")
    bool bClearBeforeBatch = false;
```

**改动后**：整段 `protected` 区段删除（或仅保留注释 "配置已迁移至 UMeetVRDanmakuDeveloperSettings"）。

需新增 include：`#include "MeetVRDanmakuDeveloperSettings.h"`（或前向声明 + cpp 中 include）。

#### `Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Private/MeetVRDanmakuDatabaseSubsystem.cpp`

**改动点**：3 处字段读取改为 DeveloperSettings 读取。

**改动 1：`RequestDanmakuBatchFromDefaultURL()`**

改动前：
```cpp
void UMeetVRDanmakuDatabaseSubsystem::RequestDanmakuBatchFromDefaultURL()
{
    RequestDanmakuBatch(DefaultServerURL);
}
```

改动后：
```cpp
void UMeetVRDanmakuDatabaseSubsystem::RequestDanmakuBatchFromDefaultURL()
{
    const UMeetVRDanmakuDeveloperSettings* Settings = UMeetVRDanmakuDeveloperSettings::Get();
    RequestDanmakuBatch(Settings ? Settings->DefaultServerURL : FString());
}
```

**改动 2：`GetNext()` 中 `bCyclePool`**

改动前：
```cpp
if (bCyclePool)
{
    QueryCursor.Add(Hash, (Cursor + 1) % Filtered.Num());
}
else
{
    RemoveById(Result.Id);
    QueryCursor.Add(Hash, 0);
}
```

改动后：
```cpp
const UMeetVRDanmakuDeveloperSettings* Settings = UMeetVRDanmakuDeveloperSettings::Get();
const bool bShouldCycle = Settings ? Settings->bCyclePool : true;
if (bShouldCycle)
{
    QueryCursor.Add(Hash, (Cursor + 1) % Filtered.Num());
}
else
{
    RemoveById(Result.Id);
    QueryCursor.Add(Hash, 0);
}
```

**改动 3：`HandleRequestComplete()` 中 `bClearBeforeBatch`**

改动前：
```cpp
if (bClearBeforeBatch)
{
    Clear();
}
```

改动后：
```cpp
const UMeetVRDanmakuDeveloperSettings* Settings = UMeetVRDanmakuDeveloperSettings::Get();
if (Settings && Settings->bClearBeforeBatch)
{
    Clear();
}
```

**include 增补**：在 cpp 顶部加 `#include "MeetVRDanmakuDeveloperSettings.h"`。

#### `Plugins/MeetVRDanmaku/Config/DefaultMeetVRDanmaku.ini`

**改动**：ini 配置段名从 `MeetVRDanmakuDatabaseSubsystem` 改为 `MeetVRDanmakuDeveloperSettings`。

**改动前**：
```ini
[/Script/Engine.GameplayTags]
+GameplayTagList=(Tag="Danmaku.System",DevComment="弹幕系统标签")

[/Script/MeetVRDanmaku.MeetVRDanmakuDatabaseSubsystem]
DefaultServerURL="http://localhost:8080/danmaku"
bCyclePool=True
bClearBeforeBatch=False
```

**改动后**：
```ini
[/Script/Engine.GameplayTags]
+GameplayTagList=(Tag="Danmaku.System",DevComment="弹幕系统标签")

[/Script/MeetVRDanmaku.MeetVRDanmakuDeveloperSettings]
DefaultServerURL="http://localhost:8080/danmaku"
bCyclePool=True
bClearBeforeBatch=False
```

### 不改动的文件

- `MeetVRDanmakuTypes.h`
- `Actors/MeetVRDanmakuActor.h/.cpp`
- `Components/MeetVRDanmakuSpawnComponent.h/.cpp`
- `MeetVRDanmakuModule.h/.cpp`
- `MeetVRDanmaku.Build.cs`（`UDeveloperSettings` 在 Engine 模块，已依赖）
- `MeetVRDanmaku.uplugin`

## 四、Assumptions & Decisions / 假设与决策

1. **解耦范围最小化**：仅 3 个数据库层配置；SpawnComponent/LevelManager 的实例级配置（DefaultLifetime/FlightSpeed/FlightMode 等）保留为 `EditAnywhere` 实例字段，不进 DeveloperSettings
2. **单类 + Category 分组**：参考 `UExperienceZoneSettings` 简洁模板，一个 DeveloperSettings 类用 `Database|Download`、`Database|Cursor` 两个 Category 分组
3. **Get() 返回 const**：数据库运行时只读配置，不需要 mutable 版本
4. **空指针兜底**：cpp 改动点都加 `Settings ? ... : 默认值` 兜底，避免 DeveloperSettings 加载失败时崩溃
5. **默认值保持**：3 个字段默认值与原 DatabaseSubsystem 完全一致（URL/bCyclePool=true/bClearBeforeBatch=false），保证行为兼容
6. **ini 段名变更**：`MeetVRDanmakuDatabaseSubsystem` → `MeetVRDanmakuDeveloperSettings`，旧段会被 UE 自动忽略（不会报错，但配置失效）
7. **与 LevelManager 计划兼容**：DeveloperSettings 不影响 `AMeetVRDanmakuLevelManager` 设计，两个计划可并行实施

## 五、Verification / 验证步骤

### 静态检查

1. `GetDiagnostics` 工具检查 `MeetVRDanmakuDeveloperSettings.h` 无 IDE 报错
2. `GetDiagnostics` 检查 `MeetVRDanmakuDatabaseSubsystem.h/.cpp` 改动后无未定义符号错误
3. 确认 cpp 中 3 处读取点都有空指针兜底

### 编译验证

```powershell
dotnet "F:\EpicGames\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" LiaoBoVREditor Win64 Development -project="F:\LiaoBo_VR\LiaoBoVR.uproject" -NoHotReload -NoLog
```

预期：MeetVRDanmaku 模块编译通过，无 error。

### 配置验证（编译后）

1. 启动 UE 编辑器 → Project Settings → Plugins → MeetVR 分类 → 应出现 "MeetVR Danmaku" 配置页
2. 修改 DefaultServerURL → 保存 → 检查 `Config/DefaultMeetVRDanmaku.ini` 段 `[/Script/MeetVRDanmaku.MeetVRDanmakuDeveloperSettings]` 是否同步更新
3. BP 调用 `RequestDanmakuBatchFromDefaultURL()` → 应使用 DeveloperSettings 中的 URL 发起 HTTP 请求
4. 修改 `bCyclePool`/`bClearBeforeBatch` → 验证 `GetNext` 与 `HandleRequestComplete` 行为变更

### 兼容性验证

- 旧 ini 段 `[/Script/MeetVRDanmaku.MeetVRDanmakuDatabaseSubsystem]` 仍存在时，UE 不会报错（仅忽略未匹配字段）
- 升级时建议手动迁移旧段到新段（用户操作）
