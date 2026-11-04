#include "FpsGrenadePickup.h"

#include "FpsPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AFpsGrenadePickup::AFpsGrenadePickup()
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
	// 引擎球体直径 100uu，缩到约 30uu 坐在地上当补给
	Body->SetRelativeScale3D(FVector(0.3f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);

	Rotating = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Rotating"));
	Rotating->RotationRate = FRotator(0.f, 180.f, 0.f);
}

void AFpsGrenadePickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected)
	{
		return;
	}
	// 拾取判定只走距离这一条：碰撞通道在蓝图侧容易被覆盖成 Block/NoCollision，
	// 重叠事件会静默不触发（20261021 金币实测坑），重叠的有效距离还比 PickupRadius 大一圈
	if (AActor* Player = FindPlayerPawn())
	{
		if (FVector::Dist(Player->GetActorLocation(), GetActorLocation()) <= PickupRadius)
		{
			NotifyCollected(Player);
		}
	}
}

void AFpsGrenadePickup::NotifyCollected(AActor* Collector)
{
	if (bCollected)
	{
		return;
	}
	AFpsPawn* Pawn = Cast<AFpsPawn>(Collector);
	if (!Pawn)
	{
		return;
	}
	bCollected = true;
	Pawn->AddGrenades(GrantCount);
	OnCollected();
	Destroy();
}

AActor* AFpsGrenadePickup::FindPlayerPawn() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	return UGameplayStatics::GetPlayerPawn(World, 0);
}
