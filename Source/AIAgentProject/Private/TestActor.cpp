// Fill out your copyright notice in the Description page of Project Settings.


#include "TestActor.h"


// Sets default values
ATestActor::ATestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 默认缩放比例
	Scale = 1.0f;

	// 默认不可拾取
	bPickable = false;

	// 默认允许被拾取
	bCanBePicked = true;

	// 默认允许被 Pawn 拾取
	bAllowPickup = true;
}

void ATestActor::SetAllowPickup(bool bInAllowPickup)
{
	// 更新拾取总开关，并同步可拾取状态与 InteractableActor 标签
	bAllowPickup = bInAllowPickup;
	bCanBePicked = bInAllowPickup;
	ApplyPickable();
}

bool ATestActor::IsAllowPickup() const
{
	// 是否允许被拾取取决于拾取总开关
	return bAllowPickup;
}

bool ATestActor::CanBePickedByPawn() const
{
	// 需要总开关与可拾取状态都开启，且拥有 InteractableActor 标签
	return bAllowPickup && CanBePicked();
}

void ATestActor::ApplyPickable()
{
	// 可拾取：打上 InteractableActor 标签；不可拾取：移除该标签
	const FName InteractableTag(TEXT("InteractableActor"));
	if (bPickable)
	{
		Tags.AddUnique(InteractableTag);
	}
	else
	{
		Tags.Remove(InteractableTag);
	}
}

bool ATestActor::IsPickable() const
{
	// 是否可被拾取取决于是否拥有 InteractableActor 标签
	return Tags.Contains(FName(TEXT("InteractableActor")));
}

void ATestActor::SetPickable(bool bInPickable)
{
	// 更新可拾取开关并同步 InteractableActor 标签
	bPickable = bInPickable;
	ApplyPickable();
}

bool ATestActor::CanBePicked() const
{
	// 只能拾取既开启了拾取开关、又处于可拾取状态的 Actor
	return bCanBePicked && IsPickable();
}

void ATestActor::ApplyScale()
{
	// 将缩放比例应用到 Actor，保持等比缩放
	SetActorScale3D(FVector(Scale, Scale, Scale));
}

void ATestActor::TestPrintDebugInfo()
{
}


