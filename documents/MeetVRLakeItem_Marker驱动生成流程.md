# MeetVRLakeItem — Marker 驱动生成 + 生命周期管理

## Summary

在用户已精简的基类（item/manager/marker 均为极简骨架，行为逻辑下沉 BP）基础上，补齐"生命周期归属"链路：
- **Manager 持有所有生成的 marker（宿主 Actor）**，负责其生命周期（跟踪 + 统一清理 + 自身 EndPlay 时清理）。
- **Marker 持有当前 item（单一 `CurrentItem`）**，负责其生命周期（重触发销毁旧项 + marker 销毁时清理 + 显式 `DestroyCurrentItem()`）。

item 的行为状态机、目标解析、视觉/音效等仍由 BP 子类实现，C++ 基类只做"骨架 + 生命周期归属"，不引入行为逻辑。

## Current State Analysis（用户已精简版本）

- `AMeetVRLakeItem`：极简骨架。仅构造函数（`bReplicates=true`）+ 空 `Tick()`。无状态机/参数/钩子。行为全交 BP 子类。
- `UMeetVRLakeItemMarkerComponent`：`TriggerSpawn()` 仅 SpawnActor item + 广播 `OnItemSpawned` + 返回 item。无目标解析、无 `InitializeAndLaunch`。持 `ItemClass`（Manager 写入或 BP 设置）+ `ShowMarker/HideMarker` 视觉钩子。
- `AMeetVRLakeItemLevelManager`：`SpawnMarkerAtLocation(FVector)` 返回 `AActor*`（宿主）；构造函数 `CreateDefaultSubobject<UMeetVRLakeItemWeightComponent>`；直接用 `WeightComponent` 做权重抽取 → spawn 宿主 → `FindComponentByClass<MarkerComponent>` → 写入 `ItemClass` → 返回宿主。
- `FMeetVRLakeItemEntry`：`ItemClass` + `Weight` + `ItemCategory`(FString)。
- `UMeetVRLakeItemWeightComponent`：纯计算，不变。

缺失：生成的 marker / item 无归属跟踪，宿主与 item 易成孤儿；manager/marker 销毁时不清理下游。

## Proposed Changes

### 1. `MeetVRLakeItemLevelManager.h`（修改）

**What**：Manager 持有生成的宿主 Actor 列表 + 统一清理 + EndPlay 自动清理。

- 新增 `UPROPERTY(BlueprintReadOnly) TArray<TObjectPtr<AActor>> SpawnedMarkers;` —— 跟踪所有生成的宿主 Actor。
- 新增 `UFUNCTION(BlueprintCallable) void ClearAllMarkers();` —— 销毁所有有效宿主（级联销毁其 marker + item）并清空数组。
- 新增 `virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;` —— Manager 退场时自动 `ClearAllMarkers()`，避免孤儿。

**Why**：Manager 是 marker 生命周期的所有者；提供显式清理 + 退场兜底。

### 2. `MeetVRLakeItemLevelManager.cpp`（修改）

**What**：实现跟踪与清理。

- `SpawnMarkerAtLocation`：成功 `FindComponentByClass` + 写入 `ItemClass` 后，`SpawnedMarkers.Add(Host);` 再 `return Host;`。
- `ClearAllMarkers()`：遍历 `SpawnedMarkers`，对非空指针 `->Destroy()`（null 跳过），最后 `SpawnedMarkers.Empty();`。
- `EndPlay`：调用 `ClearAllMarkers();` 再 `Super::EndPlay(...)`。

**级联**：`Host->Destroy()` → 宿主组件销毁 → `Marker.OnComponentDestroyed` → 销毁 `CurrentItem`。

### 3. `MeetVRLakeItemMarkerComponent.h`（修改）

**What**：Marker 持有当前 item + 清理。

- 新增 `UPROPERTY(BlueprintReadOnly) TObjectPtr<AMeetVRLakeItem> CurrentItem;` —— 本 marker 当前拥有的 item。
- 新增 `UFUNCTION(BlueprintCallable) void DestroyCurrentItem();` —— 显式销毁 `CurrentItem`（有效则 `Destroy()` 并置空）。
- 新增 `virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;` —— marker 销毁时自动销毁 `CurrentItem`。

**Why**：Marker 是 item 生命周期的所有者；重触发/销毁时兜底清理。

### 4. `MeetVRLakeItemMarkerComponent.cpp`（修改）

**What**：实现持有与清理。

- `TriggerSpawn()`：开头 `if (CurrentItem) { CurrentItem->Destroy(); CurrentItem = nullptr; }`（销毁旧 item，防泄漏）；spawn 成功后 `CurrentItem = Item;` 再广播 `OnItemSpawned` + return。
- `DestroyCurrentItem()`：`if (CurrentItem) { CurrentItem->Destroy(); CurrentItem = nullptr; }`。
- `OnComponentDestroyed(bool)`：`DestroyCurrentItem();` 再 `Super::OnComponentDestroyed(bDestroyingHierarchy);`。

### 不修改的文件

- `MeetVRLakeItem.h/.cpp`：极简骨架不变（行为在 BP）。
- `MeetVRLakeItemTypes.h`：`FMeetVRLakeItemEntry` 不变。
- `MeetVRLakeItemWeightComponent.h/.cpp`：纯计算不变。

## Assumptions & Decisions

1. **Manager 持有宿主 Actor**：与 `SpawnMarkerAtLocation` 返回 `AActor*` 一致；销毁宿主级联销毁 marker + item。不持有 MarkerComponent（避免宿主孤儿）。
2. **Marker 单一 `CurrentItem`**：同一 marker 同时只持有一个活 item；重触发先销毁旧的（防泄漏，单活模型）。需多活可子类化改列表。
3. **C++ 基类只做生命周期归属**：不引入行为状态机/目标解析/视觉逻辑（用户已下沉 BP）。`CurrentItem`/`SpawnedMarkers` 仅为跟踪 + GC 持有 + 清理。
4. **Manager EndPlay 自动清理**：Manager 退场时销毁所有生成的宿主，防孤儿。如需保留可子类覆盖或注释该行。
5. **无单 marker 卸载 API**：只提供 `ClearAllMarkers()`；宿主被外部销毁时数组留 null，`ClearAllMarkers` 跳过。保持 API 最简。
6. **清理钩子**：Manager 用 `EndPlay`；Marker 用 `OnComponentDestroyed`（宿主销毁时触发，级联到 item）。

## Verification

1. 编译 `MeetVRTemplate`（LiaoBoVREditor target）通过，无未解析符号。
2. BP：Manager 配 `MarkerHostClass` + `WeightComponent`(DataTable)。调用 `SpawnMarkerAtLocation(Loc)` → 宿主生成、`SpawnedMarkers` 含该宿主、其 Marker 的 `ItemClass` 已写入。
3. 对该 marker 调 `TriggerSpawn()` → item 生成、`CurrentItem` 被赋值、`OnItemSpawned` 广播。
4. 再次 `TriggerSpawn()` → 旧 item 销毁、新 item 生成（`CurrentItem` 更新）。
5. 调 marker `DestroyCurrentItem()` → `CurrentItem` 销毁并置空。
6. 调 Manager `ClearAllMarkers()` → 所有宿主销毁（marker + item 级联销毁）、`SpawnedMarkers` 清空。
7. 销毁 Manager（EndPlay）→ 所有宿主自动清理。
