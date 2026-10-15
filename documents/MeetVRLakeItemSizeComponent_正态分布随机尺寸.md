# MeetVRLakeItemSizeComponent — 正态分布随机尺寸

## Summary

新增一个纯计算组件 `UMeetVRLakeItemSizeComponent`（`UActorComponent`，风格对齐 `UMeetVRLakeItemWeightComponent`）：持 `MinSize`/`MaxSize` 范围，提供 `PickRandomSize()` 返回服从正态分布、clamp 到 `[Min,Max]` 的随机尺寸。挂在 item BP 上，由 item BP 在生成时调用并自行应用缩放（如 `SetActorScale3D`）。不改动 item/marker/manager 任何基类。

## Current State Analysis

- `UMeetVRLakeItemWeightComponent`（`UActorComponent`，`BlueprintSpawnableComponent`）：纯计算，持 `UDataTable` + `PickRandomEntry()`/`GetTotalWeight()`，由调用方应用。是新组件的风格模板。
- `AMeetVRLakeItem`：极简骨架（构造函数 + 空 `Tick()`，`bReplicates=true`），行为全交 BP 子类。`TriggerSpawn(APawn*)` 在 marker 中 SpawnActor item。
- marker 持 `ItemClass`、`CurrentItem`，spawn 后 `OnItemSpawned` 广播。item 自身无尺寸逻辑。

新组件与 WeightComponent 同构：纯计算、独立、零基类依赖。

## Proposed Changes

### 1. 新建 `Public/LakeItem/MeetVRLakeItemSizeComponent.h`

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MeetVRLakeItemSizeComponent.generated.h"

/**
 * 湖面物品尺寸组件：纯计算，挂 item BP 上。
 * 持 [MinSize, MaxSize] 范围，PickRandomSize() 返回正态分布、clamp 到范围内的随机尺寸。
 * 不自动应用；由 item BP 在生成时调用并自行 SetActorScale3D 等。
 */
UCLASS(Blueprintable, ClassGroup = (MeetVR), meta = (BlueprintSpawnableComponent))
class MEETVRTEMPLATE_API UMeetVRLakeItemSizeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeetVRLakeItemSizeComponent();

	/** 最小尺寸（含） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|LakeItem|Size", meta = (ClampMin = "0.0"))
	float MinSize = 0.5f;

	/** 最大尺寸（含） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MeetVR|LakeItem|Size", meta = (ClampMin = "0.0"))
	float MaxSize = 2.0f;

	/** 按正态分布取 [MinSize, MaxSize] 内随机尺寸：Mean=中点，StdDev=范围/6(3σ)，clamp 到范围。范围非正则返回 Min */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MeetVR|LakeItem|Size")
	float PickRandomSize() const;
};
```

### 2. 新建 `Private/LakeItem/MeetVRLakeItemSizeComponent.cpp`

用 Box-Muller 生成标准正态 `Z`，再 `Mean + StdDev*Z`，clamp 到 `[Lo,Hi]`。

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.
#include "LakeItem/MeetVRLakeItemSizeComponent.h"

UMeetVRLakeItemSizeComponent::UMeetVRLakeItemSizeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UMeetVRLakeItemSizeComponent::PickRandomSize() const
{
	const float Lo = FMath::Min(MinSize, MaxSize);
	const float Hi = FMath::Max(MinSize, MaxSize);
	if (Hi <= Lo)
	{
		return Lo;
	}

	// Box-Muller → 标准正态 Z
	float U1 = FMath::FRand();
	if (U1 < KINDA_SMALL_NUMBER) { U1 = KINDA_SMALL_NUMBER; }
	const float U2 = FMath::FRand();
	const float R = FMath::Sqrt(-2.0f * FMath::Loge(U1));
	const float Theta = 2.0f * PI * U2;
	const float Z = R * FMath::Cos(Theta);

	const float Mean = (Lo + Hi) * 0.5f;
	const float StdDev = (Hi - Lo) / 6.0f; // 3σ：约 99.7% 落在范围内
	const float Value = Mean + StdDev * Z;
	return FMath::Clamp(Value, Lo, Hi);
}
```

### 不修改的文件

- `MeetVRLakeItem.h/.cpp`、`MeetVRLakeItemMarkerComponent.*`、`MeetVRLakeItemLevelManager.*`、`MeetVRLakeItemTypes.h`、`MeetVRLakeItemWeightComponent.*` 均不变。

## Assumptions & Decisions

1. **纯计算 + 挂 item BP**：风格对齐 WeightComponent（`PickRandomEntry` 由调用方应用）。`PickRandomSize()` 仅返回 float；应用（如 `SetActorScale3D(FVector(Size))` 均匀缩放，或单轴）由 item BP 在 `BeginPlay`/生成事件自行决定。C++ 基类零改动。
2. **正态参数自动推导（3σ）**：`Mean=(Min+Max)/2`，`StdDev=(Max-Min)/6`，结果 `Clamp` 到 `[Min,Max]`。不暴露额外 StdDev 旋钮（最简）。
3. **Box-Muller 实现**：自包含，不依赖不确定的 UE Gaussian API。`FMath::FRand/Loge/Sqrt/Clamp/Min/Max`、`PI`、`KINDA_SMALL_NUMBER` 均由 CoreMinimal 提供。
4. **范围退化处理**：`Max<=Min` 时返回 `Min`（不报错）；`Min>Max` 时内部交换，不强制约束用户输入。
5. **命名**：`UMeetVRLakeItemSizeComponent` + `MinSize`/`MaxSize`/`PickRandomSize`（"大小/size" 语义，对应均匀缩放场景）。`UActorComponent`（纯计算无需 transform）。

## Verification

1. 编译 `MeetVRTemplate`（LiaoBoVREditor target）通过，无未解析符号。
2. item BP：添加 `MeetVRLakeItemSizeComponent`，设 `MinSize`/`MaxSize`（如 0.5/2.0）。`BeginPlay` → `PickRandomSize()` → `SetActorScale3D(FVector(Size))`。
3. 多次生成 item：尺寸集中在范围中点附近、向两端递减，且永不越界 [Min,Max]。
4. 设 `MinSize=MaxSize`：`PickRandomSize()` 恒返回该值（退化态）。
