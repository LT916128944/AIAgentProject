#include "FpsExplosion.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AFpsExplosion::AFpsExplosion()
{
	PrimaryActorTick.bCanEverTick = true;

	Blast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Blast"));
	Blast->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Blast);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Blast->SetStaticMesh(Sphere.Object);
	}
	Blast->SetRelativeScale3D(FVector(0.1f));
	Blast->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);

	// 碎片方向在构造时定死：表现要稳定，测试只断言存在与自毁，不断言轨迹
	const FVector Directions[] = {
		FVector(1.f, 0.f, 0.4f), FVector(-1.f, 0.f, 0.4f),
		FVector(0.f, 1.f, 0.4f), FVector(0.f, -1.f, 0.4f),
		FVector(0.7f, 0.7f, 0.4f), FVector(-0.7f, -0.7f, 0.4f),
		FVector(0.7f, -0.7f, 0.4f), FVector(-0.7f, 0.7f, 0.4f),
	};
	for (int32 Index = 0; Index < 8; ++Index)
	{
		UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Shard%d"), Index));
		Shard->SetMobility(EComponentMobility::Movable);
		Shard->SetupAttachment(Blast);
		if (Sphere.Succeeded())
		{
			Shard->SetStaticMesh(Sphere.Object);
		}
		Shard->SetRelativeScale3D(FVector(ShardBaseScale));
		Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shards.Add(Shard);
		ShardDirections.Add(Directions[Index].GetSafeNormal());
	}

	SetLifeSpan(LifeTime + 1.f);
}

void AFpsExplosion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Progress = FMath::Clamp(Age / LifeTime, 0.f, 1.f);
	if (Blast)
	{
		// 前半程膨胀到峰值，后半程缩到 0：一个能看懂的简易冲击波
		const float Scale = Progress < 0.5f
			? FMath::Lerp(0.1f, BlastPeakScale, Progress / 0.5f)
			: FMath::Lerp(BlastPeakScale, 0.01f, (Progress - 0.5f) / 0.5f);
		Blast->SetRelativeScale3D(FVector(Scale));
	}
	const float ShardScale = ShardBaseScale * (1.f - Progress);
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		if (UStaticMeshComponent* Shard = Shards[Index].Get())
		{
			// 位移用 Attachment 下的相对位移叠加，扫掠关掉（约定：表现不查碰撞）
			Shard->AddWorldOffset(ShardDirections[Index] * 300.f * DeltaSeconds, false);
			Shard->SetRelativeScale3D(FVector(FMath::Max(ShardScale, 0.001f)));
		}
	}
	if (Age >= LifeTime)
	{
		Destroy();
	}
}
