#include "FpsBullet.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AFpsBullet::AFpsBullet()
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
	// 引擎球体直径 100uu，缩到约 40uu，近处也能看出是一发子弹。
	Body->SetRelativeScale3D(FVector(0.4f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);
}

void AFpsBullet::Launch(const FVector& From, const FVector& To)
{
	const FVector Delta = To - From;
	const float Distance = Delta.Size();
	if (Distance < 1.f)
	{
		Destroy();
		return;
	}
	Direction = Delta / Distance;
	Remaining = Distance;
	SetActorLocation(From);
	SetActorRotation(Direction.Rotation());
	SetLifeSpan(3.f);
}

void AFpsBullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Remaining <= 0.f)
	{
		Destroy();
		return;
	}
	const float Step = FMath::Min(Speed * DeltaSeconds, Remaining);
	AddActorWorldOffset(Direction * Step, false);
	Remaining -= Step;
	if (Remaining <= 1.f)
	{
		Destroy();
	}
}
