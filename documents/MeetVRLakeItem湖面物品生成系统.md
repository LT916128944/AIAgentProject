# MeetVR 湖面物品生成系统（LakeItem）实现计划

## 摘要

在 `MeetVRTemplate` 插件内新增"湖面物品生成系统"特性：湖面上的标识以**组件**形式挂到用户既有的"操作 Actor"蓝图上（对基类不做任何假设）；外部调用触发该组件 → 组件调用关卡中的 **Manager（spawner 角色，只负责 SpawnActor 物品 + 启动飞行）**；物品按权重表（`UDataTable`）随机抽取 —— **抽取逻辑独立为** **`UMeetVRLakeItemWeightComponent`（纯计算组件）**，Manager 引用并委托其抽取。

**职责三分（已与用户确认）：**

* `UMeetVRLakeItemWeightComponent` — **权重计算**：持 `UDataTable` + `PickRandomEntry()`，纯计算、可挂任意 Actor、可复用。

* `AMeetVRLakeItemLevelManager` — **生成（spawner）**：持 `WeightComponent` 引用 + 目标解析；`SpawnActor` 物品 + 调 `InitializeAndLaunch`；不持表/不写抽取/不留默认行为参数。

* `UMeetVRLakeItemMarkerComponent` — **触发入口**：挂到任意用户"操作 Actor"BP；暴露 `TriggerSpawn()`；调用 Manager 生成物品；提供标识视觉钩子。

**精简原则（已与用户确认）：**

* `Types` 不放功能性字段（Return/Lifetime/Speed 等留给 item 对象自行设置）：`FMeetVRLakeItemEntry` 只留 `ItemClass` + `Weight`；移除 `FMeetVRLakeItemSpawnData`，改直接传 `FVector TargetLocation`。

* `AMeetVRLakeItem` 只给基础函数：自持速度/寿命/去向属性（BP 子类设默认值），状态机 + `InitializeAndLaunch` + `FinishNow` + 4 个状态钩子；不冗余。

* 数据/运动在 C++，视觉留蓝图 `BlueprintNativeEvent` override。

**其他决策（已确认）：**

* 权重表 = `UDataTable`；操作 Actor 基类目前是蓝图资产，不做假设。

**遵循的既有模式：**

* 特性文件夹组织：`Public/LakeItem/` + `Private/LakeItem/`（参照 `Weapons/`、`Arrival/`）

* 命名前缀 `MeetVR`、`ClassGroup=(MeetVR)`、`BlueprintSpawnableComponent`

* 参照 `AMeetVRWeaponSpawner`（Actor + 钩子）、`UMeetVRDanmakuComponent`（USceneComponent + 钩子）

## 现状分析

* `MeetVRTemplate.Build.cs` 已依赖 `Engine`、`Core`、`CoreUObject`、`Niagara`、`Json`、`JsonUtilities`、`DeveloperSettings`、`GameplayTags` 等，**无需改 Build.cs**。

* 既有 `AMeetVRWeaponSpawner`：AActor + 碰撞 + 冷却 + `BlueprintNativeEvent` 钩子 —— Manager 的 spawner 范式参考。

* 既有 `UMeetVRDanmakuComponent`：`USceneComponent` + 数据中间层 + `BlueprintNativeEvent` 钩子 —— Marker 组件的范式参考。

* 用户"操作 Actor 基类"目前是蓝图资产，**不做假设**；Marker 组件设计为自包含、可挂任意 AActor。

* 目标"观众"在 VR LBE 语境下即本地玩家 Pawn；Manager 提供 `TargetActor` 配置，留空自动回退 `GetPlayerPawn(this,0)`，`TargetOffset`（本地空间）表达"面前"。

## 待新增文件清单

公共头（`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/LakeItem/`）：

1. `MeetVRLakeItemTypes.h` — 枚举 + DataTable 行结构（极简）
2. `MeetVRLakeItemWeightComponent.h` — 权重计算组件（持 DataTable + 抽取）
3. `MeetVRLakeItemMarkerComponent.h` — 标识组件（触发入口 + 视觉钩子 + 关联 Manager）
4. `MeetVRLakeItem.h` — 物品 Actor（极简：自持行为参数 + 状态机 + 基础函数 + 钩子）
5. `MeetVRLakeItemLevelManager.h` — 关卡 spawner（持 WeightComponent + 目标解析 + Spawn）

实现（`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/LakeItem/`）：
6\. `MeetVRLakeItemWeightComponent.cpp`
7\. `MeetVRLakeItemMarkerComponent.cpp`
8\. `MeetVRLakeItem.cpp`
9\. `MeetVRLakeItemLevelManager.cpp`

（相比上一稿：`Types` 去掉 override 字段与 `SpawnData`；`AMeetVRLakeItem` 改为自持行为参数 + 直接接收 `FVector TargetLocation`；Manager 去掉全部默认行为配置属性。）

## 详细设计

### 1. `MeetVRLakeItemTypes.h`（极简）

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MeetVRLakeItemTypes.generated.h"

/** 物品停留结束后的去向（item 自持） */
UENUM(BlueprintType)
enum class EMeetVRLakeItemReturnMode : uint8
{
    ReturnToLake UMETA(DisplayName = "Return To Lake"),
    Dissipate     UMETA(DisplayName = "Dissipate"),
};

/** 物品运行阶段（蓝图只读驱动视觉） */
UENUM(BlueprintType)
enum class EMeetVRLakeItemPhase : uint8
{
    Idle, FlyingToTarget, Hovering, Returning, Finished,
};

/** 权重表行结构（DataTable 用）—— 仅 ItemClass + Weight */
USTRUCT(BlueprintType)
struct FMeetVRLakeItemEntry : public FTableRowBase
{
    GENERATED_BODY()
    /** 物品 Actor 类（用户 BP 子类，含网格/特效/音效 + 自持行为参数） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem")
    TSubclassOf<class AMeetVRLakeItem> ItemClass;
    /** 权重（>0 才参与抽取） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem", meta=(ClampMin="0.0"))
    float Weight = 1.0f;
};
```

> 不再含 Lifetime/ReturnMode/Speed override；不再含 `FMeetVRLakeItemSpawnData`。飞行/寿命/去向由 `AMeetVRLakeItem` 子类自行设置（见 §4）。

### 2. `MeetVRLakeItemWeightComponent.h/.cpp`（权重计算组件）

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeetVRLakeItemTypes.h"
#include "MeetVRLakeItemWeightComponent.generated.h"

class UDataTable;

/**
 * 湖面物品权重计算组件：纯计算，可挂任意 Actor。
 * 持 UDataTable（行结构 FMeetVRLakeItemEntry）并提供按权重随机抽取。
 * 不关心生成/飞行/视觉；仅做"选哪一条"的计算。
 */
UCLASS(Blueprintable, ClassGroup=(MeetVR), meta=(BlueprintSpawnableComponent))
class MEETVRTEMPLATE_API UMeetVRLakeItemWeightComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UMeetVRLakeItemWeightComponent();

    /** 权重表（行结构 FMeetVRLakeItemEntry） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem|Weight",
        meta=(RequiredAssetDataTags="RowStruct=/Script/MeetVRTemplate.MeetVRLakeItemEntry"))
    TObjectPtr<UDataTable> WeightTable;

    /** 按权重随机取一行（ItemClass 非空且 Weight>0 才参与）；空表/全 0 返回 false */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="MeetVR|LakeItem|Weight")
    bool PickRandomEntry(FMeetVRLakeItemEntry& OutEntry) const;

    /** 总权重（调试用） */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="MeetVR|LakeItem|Weight")
    float GetTotalWeight() const;
};
```

`PickRandomEntry` 实现：

1. `WeightTable` 空 → false。
2. 遍历 `WeightTable->GetRowMap()`（`TMap<FName, uint8*>`），按 `FMeetVRLakeItemEntry*` 读取，过滤 `Weight>0 && ItemClass` 非空，本地数组收集 + 累加总权重。
3. 无有效行 → false。
4. `float Roll = FMath::FRandRange(0.0f, Total)`；累加命中 → `OutEntry = *命中行`；return true。

`GetTotalWeight`：同遍历求和（Weight>0 且 ItemClass 非空）。

### 3. `MeetVRLakeItemMarkerComponent.h/.cpp`（标识组件 = 触发入口）

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MeetVRLakeItemMarkerComponent.generated.h"

class AMeetVRLakeItemLevelManager;
class AMeetVRLakeItem;

/** 标识触发完成委托（生成的物品） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLakeItemMarkerSpawned, class UMeetVRLakeItemMarkerComponent*, Marker, AMeetVRLakeItem*, Item);

/**
 * 湖面标识组件：自包含，可挂到任意用户"操作 Actor"BP 上。
 * 职责：标识视觉钩子 + 触发入口；触发后调用关联 Manager 生成物品。
 */
UCLASS(Blueprintable, ClassGroup=(MeetVR), meta=(BlueprintSpawnableComponent))
class MEETVRTEMPLATE_API UMeetVRLakeItemMarkerComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UMeetVRLakeItemMarkerComponent();

    /** 触发：调用关联 Manager 在本组件位置生成物品并飞向观众。返回生成的物品（失败 nullptr） */
    UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|Marker")
    AMeetVRLakeItem* TriggerSpawn();

    /** 关联的 Manager；空=运行时自动取关卡中第一个 Manager */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem|Marker")
    TObjectPtr<AMeetVRLakeItemLevelManager> LinkedManager;

    UPROPERTY(BlueprintAssignable, Category="MeetVR|LakeItem|Marker")
    FOnLakeItemMarkerSpawned OnItemSpawned;

    // ===== 视觉钩子（蓝图 override 加 Niagara/网格/光柱） =====
    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem|Marker")
    void ShowMarker();
    virtual void ShowMarker_Implementation() {}

    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem|Marker")
    void HideMarker();
    virtual void HideMarker_Implementation() {}

protected:
    /** 运行时解析 Manager（LinkedManager 优先，否则关卡取一个） */
    AMeetVRLakeItemLevelManager* ResolveManager() const;
};
```

`TriggerSpawn` 实现：

1. `AMeetVRLakeItemLevelManager* M = ResolveManager()`；空 → nullptr + 警告日志。
2. `AMeetVRLakeItem* Item = M->SpawnItemAtMarker(this)`。
3. `OnItemSpawned.Broadcast(this, Item)`。
4. return Item。

### 4. `MeetVRLakeItem.h/.cpp`（极简状态机）

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeetVRLakeItemTypes.h"
#include "MeetVRLakeItem.generated.h"

/**
 * 湖面物品 Actor：极简。
 * 行为参数（速度/寿命/去向）由本类自持，BP 子类设默认值；状态机驱动飞行→停留→回归/消散。
 * 视觉/音效由 4 个 BlueprintNativeEvent 钩子交蓝图。
 */
UCLASS(Blueprintable, ClassGroup=(MeetVR))
class MEETVRTEMPLATE_API AMeetVRLakeItem : public AActor
{
    GENERATED_BODY()
public:
    AMeetVRLakeItem();
    virtual void Tick(float DeltaTime) override;

    /** Manager 调用：锁定目标并启动 FlyingToTarget。spawn 位置=当前 ActorLocation。 */
    UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem")
    void InitializeAndLaunch(FVector TargetLocation);

    /** 提前结束（强制消散/归位完成） */
    UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem")
    void FinishNow();

    // ===== 自持行为参数（BP 子类设默认值） =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem", meta=(ClampMin="0.0"))
    float FlySpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem", meta=(ClampMin="0.0"))
    float ReturnSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem", meta=(ClampMin="0.1"))
    float Lifetime = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem")
    EMeetVRLakeItemReturnMode ReturnMode = EMeetVRLakeItemReturnMode::ReturnToLake;

    UPROPERTY(BlueprintReadOnly, Category="MeetVR|LakeItem")
    EMeetVRLakeItemPhase Phase = EMeetVRLakeItemPhase::Idle;

    // ===== 状态钩子（蓝图 override 视觉/音效） =====
    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem")
    void OnItemSpawned();       // 蹦出
    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem")
    void OnReachedAudience();   // 到达观众面前
    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem")
    void OnReturnStart();       // 开始回湖
    UFUNCTION(BlueprintNativeEvent, Category="MeetVR|LakeItem")
    void OnItemFinished();      // 结束（消散或归位）

private:
    FVector SpawnLocation = FVector::ZeroVector;
    FVector TargetLocation = FVector::ZeroVector;
    FTimerHandle HoverTimerHandle;
    static constexpr float ArriveThreshold = 5.0f;

    void TickFlight(float DeltaTime);
    void TickReturn(float DeltaTime);
    void EnterHovering();
    void EnterReturning();
    void Finish();
};
```

Tick 状态机（精简实现）：

* `InitializeAndLaunch(Target)`：`SpawnLocation = GetActorLocation()`；`TargetLocation = Target`；`Phase = FlyingToTarget`；`OnItemSpawned`。

* `Tick`：仅 `Phase != Idle/Finished` 推进。

  * `FlyingToTarget`：朝 `TargetLocation` 以 `FlySpeed` 移动；距离 ≤ `ArriveThreshold` → `EnterHovering()`。

  * `Returning`：朝 `SpawnLocation` 以 `ReturnSpeed` 移动；到达 → `Finish()`。

* `EnterHovering()`：`Phase = Hovering`；`OnReachedAudience`；设 `HoverTimer = Lifetime`，到时 → 按 `ReturnMode`：`ReturnToLake`→`EnterReturning()`+`OnReturnStart`；`Dissipate`→`Finish()`+`OnItemFinished`。

* `EnterReturning()`：`Phase = Returning`。

* `Finish()`：`Phase = Finished`；`SetLifeSpan(0.01)`（蓝图可在 `OnItemFinished` override 自行延迟销毁）。

* `FinishNow()`：任意阶段 → `Finish()` + `OnItemFinished`。

### 5. `MeetVRLakeItemLevelManager.h/.cpp`（spawner + 目标解析，无默认行为配置）

```cpp
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeetVRLakeItemLevelManager.generated.h"

class UMeetVRLakeItemMarkerComponent;
class UMeetVRLakeItemWeightComponent;
class AMeetVRLakeItem;

UCLASS(Blueprintable, ClassGroup=(MeetVR))
class MEETVRTEMPLATE_API AMeetVRLakeItemLevelManager : public AActor
{
    GENERATED_BODY()
public:
    AMeetVRLakeItemLevelManager();

    /** 在指定标识位置生成物品：委托 WeightComponent 抽取 → SpawnActor → InitializeAndLaunch(解析目标) */
    UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|Manager", meta=(ReturnDisplayName="Item"))
    AMeetVRLakeItem* SpawnItemAtMarker(class UMeetVRLakeItemMarkerComponent* Marker);

    /** 在任意世界位置生成物品（不依赖标识组件） */
    UFUNCTION(BlueprintCallable, Category="MeetVR|LakeItem|Manager")
    AMeetVRLakeItem* SpawnItemAtLocation(FVector WorldLocation);

    // ===== 配置 =====
    /** 关联的权重计算组件；空=运行时在本 Actor 上自动查找一个 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem|Manager")
    TObjectPtr<UMeetVRLakeItemWeightComponent> WeightComponent;

    /** 观众目标 Actor；空=运行时取本地玩家 Pawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem|Manager")
    TObjectPtr<AActor> TargetActor;

    /** "面前"本地偏移（相对 TargetActor 朝向；Forward=前方） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MeetVR|LakeItem|Manager")
    FVector TargetOffset = FVector(150.0f, 0.0f, 0.0f);

protected:
    /** 解析 WeightComponent（显式优先；空则本 Actor 上 GetComponentByClass 兜底） */
    UMeetVRLakeItemWeightComponent* ResolveWeightComponent() const;
    /** 解析目标世界坐标：TargetActor.Location + RotateOffset(Rotation, TargetOffset)；空回退本地玩家 Pawn */
    FVector ResolveTargetWorldLocation() const;
};
```

`SpawnItemAtMarker` 实现要点：

1. 校验 `Marker` 非空
2. `UMeetVRLakeItemWeightComponent* WC = ResolveWeightComponent()`；空 → nullptr + 警告日志
3. `FMeetVRLakeItemEntry Entry;` → `WC->PickRandomEntry(Entry)` 失败 → nullptr + 警告日志
4. `FVector Loc = Marker->GetComponentLocation()`
5. `AMeetVRLakeItem* Item = GetWorld()->SpawnActor<AMeetVRLakeItem>(Entry.ItemClass, Loc, FRotator::ZeroRotator)`
6. `Item->InitializeAndLaunch(ResolveTargetWorldLocation())`
7. return Item

`SpawnItemAtLocation(WorldLocation)`：同流程，Loc 用入参，不需 Marker。

`ResolveWeightComponent`：

```cpp
if (WeightComponent) return WeightComponent;
TArray<UMeetVRLakeItemWeightComponent*> Comps;
GetComponents(Comps);
return Comps.Num() > 0 ? Comps[0] : nullptr;
```

`ResolveTargetWorldLocation`：

```cpp
AActor* T = TargetActor ? TargetActor : UGameplayStatics::GetPlayerPawn(this, 0);
if (!T) return FVector::ZeroVector;
return T->GetActorLocation() + T->GetActorRotation().RotateVector(TargetOffset);
```

## 假设与决策

* **职责三分**：WeightComponent=计算（DataTable+抽取）、Manager=生成（Spawn+目标解析+委托抽取）、MarkerComponent=触发入口。

* **Manager 不持 DataTable、不持默认行为参数**：权重表在 WeightComponent；速度/寿命/去向在 `AMeetVRLakeItem` 自持；Manager 只剩 `WeightComponent`+`TargetActor`+`TargetOffset` 三项配置。

* **Types 极简**：`FMeetVRLakeItemEntry` 仅 `ItemClass`+`Weight`；移除 `FMeetVRLakeItemSpawnData`，`InitializeAndLaunch` 直接收 `FVector TargetLocation`。

* **行为参数留给 item 对象**：`FlySpeed`/`ReturnSpeed`/`Lifetime`/`ReturnMode` 在 `AMeetVRLakeItem` 上，BP 子类设默认值；不同物品类型可有不同飞行特性。

* **WeightComponent 自动解析**：Manager 的 `WeightComponent` 留空时，运行时在本 Actor 上 `GetComponentByClass` 兜底（典型：挂到 Manager 自己身上）。

* **不引入 Subsystem/Database/网络复制**：静态 DataTable、单机演够用。

* **操作 Actor 基类不做假设**：Marker 组件自包含，可挂任意用户 BP Actor。

* **权重表 =** **`UDataTable`**：行结构 `FMeetVRLakeItemEntry`；`WeightTable` 用 `RequiredAssetDataTags` 限定行结构。

* **"观众面前"= 本地玩家 Pawn + 本地偏移**：`TargetOffset` 本地空间，`FVector(150,0,0)` 默认"前方 1.5m"。

* **Item 销毁**：状态机结束默认 `SetLifeSpan(0.01)`；蓝图 override `OnItemFinished` 可自行延迟销毁。

* **空/全 0 权重表**：`PickRandomEntry` 返回 false → `SpawnItemAtMarker` 返回 nullptr + 警告，不崩溃。

* **不改 Build.cs**：依赖已齐全。

* **无冷却/节流**：按"仅实现功能"原则不加。

## 验证步骤

1. **编译**：`MeetVRTemplate` 模块编译通过（无 Build.cs 改动）。
2. **DataTable 配置**：新建 `FMeetVRLakeItemEntry` 行结构的 DataTable，填 2-3 行（不同 `ItemClass` + Weight）。
3. **BP 子类**：`AMeetVRLakeItem` 的 BP 子类（加 StaticMesh + 设 `FlySpeed`/`Lifetime`/`ReturnMode` 等默认值）；`UMeetVRLakeItemMarkerComponent`/`UMeetVRLakeItemWeightComponent` 基类已可挂。
4. **关卡装配**：

   * 拖入一个 `AMeetVRLakeItemLevelManager`，挂一个 `UMeetVRLakeItemWeightComponent`（设其 `WeightTable`），配 `TargetOffset`。

   * 拖入若干"操作 Actor"BP（用户的既有蓝图），每个挂 `UMeetVRLakeItemMarkerComponent`，设 `LinkedManager`（或留空自动取）。
5. **触发测试**：关卡蓝图按数字键调用某 MarkerComponent 的 `TriggerSpawn()`。
6. **行为校验**：

   * 物品在该标识位置出现并飞向玩家面前（约 `TargetOffset` 距离处）。

   * 停留 item 自设的 `Lifetime` 秒后：`ReturnToLake` 飞回标识位置再销毁；`Dissipate` 直接销毁。

   * 多次触发，按 DataTable 权重概率出现不同 `ItemClass`。
7. **蓝图钩子**：在 Item BP override `OnItemSpawned`/`OnReachedAudience`/`OnReturnStart`/`OnItemFinished`；在 MarkerComponent 挂载处 override `ShowMarker/HideMarker` 确认调用时机。
8. **WeightComponent 复用性**：挂到别的 Actor 也能独立调 `PickRandomEntry`（验证计算与生成解耦）。

