# MeetVRDanmaku 关卡管理器设计计划

## 一、Summary / 摘要

在 `MeetVRDanmaku` 插件中新增关卡级管理器 `AMeetVRDanmakuLevelManager`（AActor，可拖入关卡），用于在关卡 BeginPlay 时（或 BP 手动触发时）从 `UMeetVRDanmakuDatabaseSubsystem` 按 `SpaceId`+`SectionId` 查询本关相关弹幕数据，**一次性批量生成所有弹幕 Actor**。

设计原则：**完全不改动用户已有的修改**（Types/DatabaseSubsystem/Actor/SpawnComponent 等），仅新增 2 个文件。管理器独立持有生成配置（与 SpawnComponent 镜像但不依赖），自行 `SpawnActor` + `Initialize`。

## 二、Current State Analysis / 现状分析

### 已有代码现状（用户已修改，不得改动）

| 文件 | 用户修改点（保留） |
|------|---------------------|
| `MeetVRDanmakuTypes.h` | 完整定义 `FMeetVRDanmakuItem` / `FMeetVRDanmakuSpawnData` / `FMeetVRDanmakuQuery` / 枚举 |
| `MeetVRDanmakuDatabaseSubsystem.h/cpp` | `UCLASS()` 无 DisplayName；`DefaultServerURL/bCyclePool/bClearBeforeBatch` 改为 `EditDefaultsOnly`（去掉 Config）；JSON 反序列化用 `TJsonReaderFactory::Create`（非 CreateFromRootString） |
| `MeetVRDanmakuActor.h/cpp` | `SpawnData` 加 `UPROPERTY()`；其余不变 |
| `MeetVRDanmakuSpawnComponent.h/cpp` | 基类改为 `USceneComponent`（不再是 `UActorComponent`），保留 `SpawnItem(Item)` / `BuildSpawnData(Item)` / `TriggerSpawn()` |

### 数据库可复用能力

- `Query(const FMeetVRDanmakuQuery&)` → `TArray<FMeetVRDanmakuItem>`：**非消费式**查询，可重复调用，正好满足"访问关卡内相关弹幕数据"
- `GetNext/GetNextBySpaceSection`：消费式取下一条，**不适合**批量生成（带游标）
- `FMeetVRDanmakuQuery`：`SpaceId`/`SectionId`/`bFilterType`/`Type` 字段，空字符串=通配

### Actor 生成可复用能力

- `AMeetVRDanmakuActor::Initialize(const FMeetVRDanmakuSpawnData&)`：已封装位置/速度/生命周期/延迟赋值
- `FMeetVRDanmakuSpawnData`：完整生成数据载荷
- `SpawnComponent::BuildSpawnData/ComputeVelocity/ComputeSpawnLocation/ComputeSpawnRotation`：现成合并逻辑（Item 覆盖默认）

按用户决策"独立持配置"，管理器**自行重新实现**等价的 `BuildSpawnData` 等（与 SpawnComponent 平行实现，不引用 SpawnComponent），避免跨类耦合。

### 项目同类参考（来自 Phase 1 探索）

- `MeetVRTemplate/MeetVRExperienceManagerComponent`（UGameStateComponent）：体验加载管理
- `MeetVRTemplate/MeetVRPlayerSpawningManagerComponent`（UGameStateComponent）：玩家出生管理

用户选择 AActor 而非 UGameStateComponent，理由更直观：管理器可拖入关卡，BP 配置即时可见，不需要修改 GameMode。

## 三、Proposed Changes / 提案变更

### 新增文件

#### 1. `Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Public/Actors/MeetVRDanmakuLevelManager.h`

**职责**：关卡级弹幕批量管理器

**关键接口**：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeetVRDanmakuTypes.h"
#include "MeetVRDanmakuLevelManager.generated.h"

class AMeetVRDanmakuActor;
class UMeetVRDanmakuDatabaseSubsystem;

/** SpawnAll 完成委托（参数：实际生成数量） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMeetVRDanmakuLevelManagerSpawnAll, int32, SpawnedCount);

/**
 * 弹幕关卡管理器。
 * 关卡级单例（AActor），可拖入关卡；BeginPlay 时按 SpaceId+SectionId 查询数据库并批量生成所有弹幕 Actor。
 * 数据来源：UMeetVRDanmakuDatabaseSubsystem::Query（非消费式，可重复 SpawnAll）。
 */
UCLASS(Blueprintable, ClassGroup = (MeetVR))
class MEETVRDANMAKU_API AMeetVRDanmakuLevelManager : public AActor
{
    GENERATED_BODY()

public:
    AMeetVRDanmakuLevelManager();

    virtual void BeginPlay() override;

    /**
     * 从数据库按 SpaceId+SectionId（可选 Type 过滤）查询所有 Item，
     * 按 Timestamp 升序逐个 SpawnActor + Initialize。
     * bClearExistingActorsBeforeSpawn=true 时先清理本管理器已生成的 Actor。
     * 返回实际生成数量。
     */
    UFUNCTION(BlueprintCallable, Category = "MeetVR|Danmaku|LevelManager", meta = (ReturnDisplayName = "Spawned Count"))
    int32 SpawnAll();

    /** 销毁本管理器生成的所有还存活的弹幕 Actor */
    UFUNCTION(BlueprintCallable, Category = "MeetVR|Danmaku|LevelManager")
    void ClearSpawnedActors();

    /** 获取当前还存活的生成 Actor 列表（自动清理已销毁的） */
    UFUNCTION(BlueprintPure, Category = "MeetVR|Danmaku|LevelManager")
    TArray<AMeetVRDanmakuActor*> GetSpawnedActors();

    // ===== 查询过滤 =====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager")
    FString SpaceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager")
    FString SectionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager")
    bool bFilterType = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager", meta = (EditCondition = "bFilterType"))
    EMeetVRDanmakuType FilterType = EMeetVRDanmakuType::Custom;

    // ===== 生命周期 =====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager")
    bool bAutoSpawnOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager")
    bool bClearExistingActorsBeforeSpawn = false;

    // ===== 生成配置（与 SpawnComponent 镜像，独立持配置） =====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn")
    TSubclassOf<AMeetVRDanmakuActor> DanmakuActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn", meta = (ClampMin = "0.1"))
    float DefaultLifetime = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn", meta = (ClampMin = "0.0"))
    float DefaultFlightSpeed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn")
    EMeetVRDanmakuFlightMode FlightMode = EMeetVRDanmakuFlightMode::Upward;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn", meta = (EditCondition = "FlightMode == EMeetVRDanmakuFlightMode::CustomDirection"))
    FVector CustomDirection = FVector(0.0f, 0.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|Danmaku|LevelManager|Spawn")
    FVector SpawnOffset = FVector::ZeroVector;

    // ===== 委托 =====

    UPROPERTY(BlueprintAssignable, Category = "MeetVR|Danmaku|LevelManager")
    FOnMeetVRDanmakuLevelManagerSpawnAll OnSpawnAllComplete;

protected:
    // 与 SpawnComponent 等价的合并逻辑，以管理器自身位置为锚点
    FMeetVRDanmakuSpawnData BuildSpawnData(const FMeetVRDanmakuItem& Item) const;
    FVector ComputeVelocity(float Speed) const;
    FVector ComputeSpawnLocation(const FMeetVRDanmakuItem& Item) const;
    FRotator ComputeSpawnRotation(const FMeetVRDanmakuItem& Item) const;

    /** 单条 Item SpawnActor + Initialize；返回生成的 Actor（失败返回 nullptr） */
    AMeetVRDanmakuActor* SpawnOne(const FMeetVRDanmakuItem& Item);

private:
    /** 当前管理器生成且仍存活的 Actor 列表 */
    UPROPERTY(Transient)
    TArray<TObjectPtr<AMeetVRDanmakuActor>> SpawnedActors;

    /** 缓存数据库（BeginPlay 取一次，运行时懒加载兜底） */
    UPROPERTY(Transient)
    TObjectPtr<UMeetVRDanmakuDatabaseSubsystem> CachedDB = nullptr;

    UMeetVRDanmakuDatabaseSubsystem* GetDatabase();
    void PruneDestroyedActors();
};
```

#### 2. `Plugins/MeetVRDanmaku/Source/MeetVRDanmaku/Private/Actors/MeetVRDanmakuLevelManager.cpp`

**实现要点**：

- **构造函数**：`PrimaryActorTick.bCanEverTick = false`（管理器本身不 tick，Actor 自己会 tick）；创建 `USceneComponent` 作 Root（提供位置）
- **BeginPlay**：缓存 DB；若 `bAutoSpawnOnBeginPlay=true` 调用 `SpawnAll()`
- **GetDatabase**：懒加载（CachedDB 兜底）
- **SpawnAll**：
  1. 取 DB；构造 `FMeetVRDanmakuQuery`（SpaceId/SectionId/bFilterType+Type）
  2. `Items = DB->Query(Filter)`
  3. 按 `Timestamp` 升序排序
  4. `bClearExistingActorsBeforeSpawn` 为真时 `ClearSpawnedActors()`
  5. 循环 `SpawnOne(Item)`，统计成功数
  6. `PruneDestroyedActors()`，广播 `OnSpawnAllComplete(Count)`
  7. 返回 Count
- **SpawnOne**：
  1. 检查 `Item.IsValidItem()` / `DanmakuActorClass` / `World`
  2. `BuildSpawnData(Item)`
  3. `SpawnParams`：`Owner=this`, `Instigator=Cast<APawn>(this)`（保险）
  4. `World->SpawnActor<AMeetVRDanmakuActor>(...)`
  5. `Actor->Initialize(Data)`
  6. 加入 `SpawnedActors`
  7. 返回 Actor
- **BuildSpawnData / ComputeVelocity / ComputeSpawnLocation / ComputeSpawnRotation**：与 SpawnComponent.cpp 完全等价的实现（只是锚点从 `GetComponentLocation()` 改为 `GetActorLocation()` / `GetActorRotation()` / `GetActorTransform().GetUnitAxis(EAxis::Z)`）
- **PruneDestroyedActors**：遍历 `SpawnedActors`，移除 `!IsValid()` 的（Actor Expire 后 Destroy 自动失效）
- **ClearSpawnedActors**：对每个有效 Actor 调用 `Destroy()`，然后 `SpawnedActors.Empty()`
- **GetSpawnedActors**：先 `PruneDestroyedActors()` 再返回数组

### 不改动的文件（保护用户修改）

- `MeetVRDanmakuTypes.h`
- `MeetVRDanmakuDatabaseSubsystem.h/.cpp`
- `Actors/MeetVRDanmakuActor.h/.cpp`
- `Components/MeetVRDanmakuSpawnComponent.h/.cpp`
- `MeetVRDanmakuModule.h/.cpp`
- `MeetVRDanmaku.Build.cs`（无需新增依赖模块，HTTP/Json/Core/Engine/CoreUObject/GameplayTags 已够用；管理器只需 Engine + GameFramework 已在依赖中）
- `MeetVRDanmaku.uplugin`
- `Config/DefaultMeetVRDanmaku.ini`

## 四、Assumptions & Decisions / 假设与决策

1. **数据库非消费查询**：用 `Query(Filter)` 而非 `GetNext`，保证可重复 SpawnAll 不消耗数据
2. **生成顺序**：按 `Timestamp` 升序（与 `GetNext` 内部排序一致），保证时序符合服务器预期
3. **生命周期管理**：Actor `Expire()` 自带 `Destroy()`，管理器只需维护弱列表，定期 `PruneDestroyedActors()`
4. **位置锚点**：管理器自身 Actor 位置作默认生成锚点（与 SpawnComponent 的组件位置对应）
5. **重复 SpawnAll 安全性**：`bClearExistingActorsBeforeSpawn` 默认 false，BP 可改 true 实现重置生成
6. **配置镜像 SpawnComponent**：DefaultLifetime/DefaultFlightSpeed/FlightMode/CustomDirection/SpawnOffset 字段名与 SpawnComponent 保持一致，便于 BP 用户跨类使用直觉一致
7. **不依赖 SpawnComponent**：完全独立实现，避免要求关卡里必须存在 SpawnComponent
8. **EditCondition meta**：`FlightMode == EMeetVRDanmakuFlightMode::CustomDirection` 跟 SpawnComponent 一致写法

## 五、Verification / 验证步骤

### 静态检查

1. 用 `GetDiagnostics` 工具检查新建头文件无 IDE 报错
2. 文件结构：`Actors/MeetVRDanmakuLevelManager.h` 与 `Actors/MeetVRDanmakuActor.h` 同目录
3. include 路径：源文件 `#include "Actors/MeetVRDanmakuLevelManager.h"` + `"MeetVRDanmakuDatabaseSubsystem.h"` + `"Actors/MeetVRDanmakuActor.h"` + `"Engine/World.h"` + `"GameFramework/Pawn.h"`

### 编译验证

执行命令（沙箱 + 项目缺 PICOXR 插件需用户在自有环境跑）：

```powershell
dotnet "F:\EpicGames\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" LiaoBoVREditor Win64 Development -project="F:\LiaoBo_VR\LiaoBoVR.uproject" -NoHotReload -NoLog
```

预期：MeetVRDanmaku 模块编译通过，无 error。

### 功能验证（编译后 BP 流程）

1. 创建 BP 子类 `BP_MeetVRDanmakuLevelManager` 继承 `AMeetVRDanmakuLevelManager`
2. 在 BP 设置 `DanmakuActorClass` 为已有弹幕 BP 子类
3. 拖入关卡，设置 `SpaceId`/`SectionId`
4. Play In Editor：
   - `bAutoSpawnOnBeginPlay=true` 时 BeginPlay 应自动生成所有匹配 Item 的 Actor
   - 调用 `SpawnAll()` 测试手动重新生成
   - 调用 `ClearSpawnedActors()` 测试清理
   - `GetSpawnedActors()` 应返回当前存活列表
