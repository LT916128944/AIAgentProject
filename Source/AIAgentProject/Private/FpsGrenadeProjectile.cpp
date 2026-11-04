#include "FpsGrenadeProjectile.h"

#include "FpsExplosion.h"
#include "FpsHealthComponent.h"
#include "FpsTarget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AFpsGrenadeProjectile::AFpsGrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Body);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Body->SetStaticMesh(Sphere.Object);
	}
	// 引擎球体直径 100uu，缩到约 50uu，比子弹大一圈，看得清是颗雷
	Body->SetRelativeScale3D(FVector(0.5f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);

	// 主表现指向 /Game/Effects/NS_Explosion；资产没建好时路径解析失败，走兜底
	ExplosionTemplate = TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Effects/NS_Explosion.NS_Explosion")));

	SetLifeSpan(FuseSeconds + 5.f);
}

void AFpsGrenadeProjectile::Launch(const FVector& From, const FVector& InVelocity)
{
	SetActorLocation(From);
	Velocity = InVelocity;
}

void AFpsGrenadeProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bExploded)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FuseAge += DeltaSeconds;
	// 抛物线自积分：位移不靠阻挡（约定 AddActorWorldOffset sweep=false），弹体本身不进物理
	Velocity.Z -= Gravity * DeltaSeconds;
	const FVector Old = GetActorLocation();
	const FVector New = Old + Velocity * DeltaSeconds;

	// 落地/命中即爆：球体扫掠被挡住就当场爆，不让雷穿墙穿地等引信
	FCollisionQueryParams Params(TEXT("FpsGrenade"), false);
	Params.AddIgnoredActor(this);
	if (AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
	}
	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(
		Hit, Old, New, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(24.f), Params);
	if (bBlocked && Hit.bBlockingHit)
	{
		SetActorLocation(FVector(Hit.ImpactPoint) + FVector(Hit.ImpactNormal) * 24.f);
		Explode();
		return;
	}
	AddActorWorldOffset(New - Old, false);
	if (FuseAge >= FuseSeconds)
	{
		Explode();
	}
}

void AFpsGrenadeProjectile::Explode()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FVector Center = GetActorLocation();

	// 半径内反馈只走距离判定：Block 会盖掉 Overlap（20261021 实测坑），口径与金币一致
	for (TActorIterator<AFpsTarget> It(World); It; ++It)
	{
		AFpsTarget* Target = *It;
		if (!Target || !Target->Health)
		{
			continue;
		}
		if (FVector::Dist(Target->GetActorLocation(), Center) > BlastRadius)
		{
			continue;
		}
		Target->Health->ApplyDamage(BlastDamage);
		Target->ReactToHit();
	}

	SpawnVisual(Center);
	Destroy();
}

void AFpsGrenadeProjectile::SpawnVisual(const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (UNiagaraSystem* System = ExplosionTemplate.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, System, Location, FRotator::ZeroRotator, FVector(1.f),
			/*bAutoDestroy=*/true, /*bAutoActivate=*/true);
		return;
	}
	// NS_Explosion 还没建：几何球兜底，逻辑链路照常可测
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AFpsExplosion>(Location, FRotator::ZeroRotator, Params);
}
