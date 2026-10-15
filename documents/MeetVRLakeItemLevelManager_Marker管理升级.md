# MeetVRLakeItemLevelManager — Marker 管理（SuckerActorSpawner 管理子集）

## Summary

在 `AMeetVRLakeItemLevelManager` 上实现 SuckerActorSpawner 式的"管理"能力，应用于**已生成的 marker（宿主 Actor）**——不直接 spawn/manage item（item 仍由 marker 自管）。仅管理子集（无 TargetCount/SpawnInterval/Start-Stop/Tick 自动维持）：跟踪 `SpawnedMarkers` + `OnDestroyed` 绑定 + 惰性 prune + 清理/查询/事件。`SpawnMarkerAtLocation` 仍为唯一手动 spawn 入口，生成时绑定 `OnDestroyed`、累加 `TotalSpawnedCount`、广播 `OnMarkerSpawned`。

## Current State Analysis

- `AMeetVRLakeItemLevelManager`：`SpawnMarkerAtLocation(FVector)` → 权重抽取 + spawn 宿主 + 写 `ItemClass` + `SpawnedMarkers.Add`；`ClearAllMarkers()` 销毁全部 + 清空；`EndPlay` → `ClearAllMarkers`；持 `MarkerHostClass`、`WeightComponent`（默认子对象）、`SpawnedMarkers`（BlueprintReadOnly，非 Transient）。
- 参考模板 `USuckerActorSpawnerComponent`：`SpawnedActors` + `OnDestroyed` 绑定 + `CleanupDestroyedActors`/`DestroyAllSpawned`/`ClearTrackedActors` + 查询（Count/List/Random/Total）+ 事件（OnActorSpawned/OnActorDestroyed）+ `TotalSpawnedCount`。**不含**其 TargetCount/SpawnInterval/Start/Stop/Tick/area（用户已确认不要自动维持）。
- marker 仍自管 item（`CurrentItem` + `OnComponentDestroyed` 清理），不受本改动影响。

## Proposed Changes

### 1. `MeetVRLakeItemLevelManager.h`（修改）

新增委托（class 前）：
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLakeItemLevelManagerMarkerSpawned, AActor*, SpawnedMarker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLakeItemLevelManagerMarkerDestroyed, AActor*, DestroyedMarker);
```

类内新增/调整：
```cpp
public:
    // 既有：SpawnMarkerAtLocation, ClearAllMarkers, MarkerHostClass

    /** 清空跟踪列表但不销毁（手动卸管；先解绑 OnDestroyed 再清空） */
    UFUNCTION(BlueprintCallable, Category = "MeetVR|LakeItem|Manager")
    void ClearTrackedMarkers();

    // ===== 查询（惰性 prune 后返回） =====
    UFUNCTION(BlueprintCallable, Category = "MeetVR|LakeItem|Manager")
    int32 GetSpawnedMarkerCount();                 // prune → Num
    UFUNCTION(BlueprintCallable, Category = "MeetVR|LakeItem|Manager")
    TArray<AActor*> GetSpawnedMarkers();            // prune → 拷贝有效
    UFUNCTION(BlueprintCallable, Category = "MeetVR|LakeItem|Manager")
    AActor* GetRandomSpawnedMarker();               // prune → 随机有效
    UFUNCTION(BlueprintPure, Category = "MeetVR|LakeItem|Manager")
    int32 GetTotalSpawnedCount() const { return TotalSpawnedCount; }

    // ===== 事件 =====
    UPROPERTY(BlueprintAssignable, Category = "MeetVR|LakeItem|Manager")
    FOnLakeItemLevelManagerMarkerSpawned OnMarkerSpawned;
    UPROPERTY(BlueprintAssignable, Category = "MeetVR|LakeItem|Manager")
    FOnLakeItemLevelManagerMarkerDestroyed OnMarkerDestroyed;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 移除已销毁/无效的宿主引用（惰性清理） */
    void PruneDestroyedMarkers();
    /** 宿主 OnDestroyed 回调：广播 OnMarkerDestroyed（不在回调内改数组） */
    UFUNCTION()
    void HandleSpawnedMarkerDestroyed(AActor* DestroyedMarker);

    // ===== 状态 =====
    UPROPERTY(Transient, BlueprintReadOnly, Category = "MeetVR|LakeItem|Manager")
    TArray<TObjectPtr<AActor>> SpawnedMarkers;          // 改为 Transient

    UPROPERTY(Transient, BlueprintReadOnly, Category = "MeetVR|LakeItem|Manager")
    int32 TotalSpawnedCount = 0;                        // 累计生成数（不清零）
};
```

### 2. `MeetVRLakeItemLevelManager.cpp`（修改）

- `SpawnMarkerAtLocation`：成功写入 `ItemClass` 后：
  ```cpp
  Host->OnDestroyed.AddDynamic(this, &AMeetVRLakeItemLevelManager::HandleSpawnedMarkerDestroyed);
  SpawnedMarkers.Add(Host);
  ++TotalSpawnedCount;
  OnMarkerSpawned.Broadcast(Host);
  return Host;
  ```
- `ClearAllMarkers()`（既有，保持销毁语义，镜像 `DestroyAllSpawned`）：遍历有效则 `Destroy()`（回调会广播 `OnMarkerDestroyed`，不改数组故安全），最后 `SpawnedMarkers.Empty()`。
- `ClearTrackedMarkers()`（新）：遍历有效则 `OnDestroyed.RemoveDynamic(this, &HandleSpawnedMarkerDestroyed)`，再 `SpawnedMarkers.Empty()`（不销毁）。
- `PruneDestroyedMarkers()`：`SpawnedMarkers.RemoveAll([](const TObjectPtr<AActor>& A){ return !A || !IsValid(A); });`
- `HandleSpawnedMarkerDestroyed(AActor*)`：`if (!IsValid(DestroyedMarker)) return; OnMarkerDestroyed.Broadcast(DestroyedMarker);`（**不在回调内 prune/改数组**，惰性 prune 留给查询/清理）。
- 查询实现：先 `PruneDestroyedMarkers()` 再返回 `Num`/拷贝/随机。
- `EndPlay`：**先解绑所有 `OnDestroyed`**（避免回调访问正在销毁的 Manager，对齐 SuckerActorSpawner EndPlay 安全处理），再 `ClearAllMarkers()`，再 `Super::EndPlay`。

### 不修改的文件

- `MeetVRLakeItemMarkerComponent.*`、`MeetVRLakeItem.*`、`MeetVRLakeItemTypes.h`、`MeetVRLakeItemWeightComponent.*`、`MeetVRLakeItemSizeComponent.*` 均不变。

## Assumptions & Decisions

1. **仅管理 marker，不碰 item**：Manager 不直接 spawn/manage item（用户确认）；item 仍由 marker 的 `CurrentItem` + `OnComponentDestroyed` 自管。本改动只升级 marker 管理。
2. **无自动维持**：不引入 TargetCount/SpawnInterval/Start/Stop/Tick/area（用户确认）。`SpawnMarkerAtLocation(FVector)` 仍为唯一手动 spawn 入口。
3. **管理子集**：跟踪(`SpawnedMarkers`+`TotalSpawnedCount`) + `OnDestroyed` 绑定 + 惰性 prune + `ClearAllMarkers`(销毁)/`ClearTrackedMarkers`(仅清空) + 查询(Count/List/Random/Total) + 事件(`OnMarkerSpawned`/`OnMarkerDestroyed`)。
4. **`OnDestroyed` 绑定**：spawn 时 `AddDynamic`；`EndPlay` 先解绑再 `ClearAllMarkers`，避免回调进入半销毁 Manager（对齐 SuckerActorSpawner 的 EndPlay 安全顾虑）。
5. **惰性 prune**：回调内不改数组（只广播），prune 在查询/`ClearAllMarkers` 时做——避免迭代-修改冲突。
6. **`TotalSpawnedCount` 不随 `ClearAllMarkers` 清零**：累计值，对齐 SuckerActorSpawner `TotalSpawnedCount`。
7. **`SpawnedMarkers` 改 Transient**：运行时填充的 actor 引用不应序列化（修正既有遗漏）。
8. **命名**：委托 `FOnLakeItemLevelManagerMarkerSpawned/Destroyed`；事件 `OnMarkerSpawned/OnMarkerDestroyed`；与 marker 的 `OnItemSpawned`(marker→item) 区分清楚。

## Verification

1. 编译 `MeetVRTemplate`（LiaoBoVREditor target）通过。
2. `SpawnMarkerAtLocation(Loc)` → 宿主生成、`SpawnedMarkers` 含它、`TotalSpawnedCount++`、`OnMarkerSpawned` 广播。
3. 外部 `Destroy()` 一个宿主 → `OnMarkerDestroyed` 广播；之后任一查询触发 prune，`GetSpawnedMarkerCount()` 下降。
4. `GetSpawnedMarkers()`/`GetRandomSpawnedMarker()` 仅返回有效宿主。
5. `ClearAllMarkers()` → 全部宿主销毁、`SpawnedMarkers` 清空、`TotalSpawnedCount` 保留。
6. `ClearTrackedMarkers()` → 列表清空但宿主仍在关卡通活（仅卸管）。
7. Manager `EndPlay`（含 LevelTransition）→ 先解绑再销毁，无回调进入半销毁 Manager。
