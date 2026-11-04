#include "FpsHUDWidget.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FpsPawn.h"
#include "SimpleVehicleTest.h"
#include "TimerManager.h"

namespace
{
	// 「按 E 上车/下车」的提示文案。载具侧 E 的行为在 AFpsPawn::TryEnterVehicle /
	// ASimpleVehiclePawn::ExitVehicle，这里只负责显示，不绑键。
	const TCHAR* const EnterPromptText = TEXT("按 E 上车");
	const TCHAR* const ExitPromptText = TEXT("按 E 下车");

	// HUD 找载具的距离，与 AFpsPawn::TryEnterVehicle 的 400uu 同一口径。
	constexpr float VehicleScanRadius = 400.f;

	// uu/s → km/h：1 uu = 1 cm，1 cm/s = 0.036 km/h。
	constexpr float UuPerSecToKmh = 0.036f;
}

void UFpsHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		// 与 UPlatformHUDWidget 同一套读法：0.05 秒定时器，不依赖蓝图 Tick 开关。
		World->GetTimerManager().SetTimer(
			RefreshTimer, this, &UFpsHUDWidget::Refresh, 0.05f, /*bLoop=*/true);
	}
	Refresh();
}

void UFpsHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
	}
	Super::NativeDestruct();
}

APawn* UFpsHUDWidget::PlayerPawn() const
{
	APlayerController* Player = GetOwningPlayer();
	if (!Player)
	{
		return nullptr;
	}
	return Player->GetPawn();
}

bool UFpsHUDWidget::IsInVehicle() const
{
	return Cast<ASimpleVehiclePawn>(PlayerPawn()) != nullptr;
}

ASimpleVehiclePawn* UFpsHUDWidget::NearestVehicle() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	const APawn* Pawn = PlayerPawn();
	const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
	ASimpleVehiclePawn* Best = nullptr;
	float BestDistSq = VehicleScanRadius * VehicleScanRadius;
	for (TActorIterator<ASimpleVehiclePawn> It(World); It; ++It)
	{
		ASimpleVehiclePawn* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}
	return Best;
}

float UFpsHUDWidget::GetDisplayedSpeedKmh() const
{
	// 开的是载具就读载具自己的速度；人在地上但车在旁边时显示 0（还没上车）。
	const ASimpleVehiclePawn* Vehicle = Cast<ASimpleVehiclePawn>(PlayerPawn());
	if (!Vehicle)
	{
		return 0.f;
	}
	return FMath::Abs(Vehicle->GetSpeedUuPerSec()) * UuPerSecToKmh;
}

const AFpsPawn* UFpsHUDWidget::FpsPawnForDisplay() const
{
	// 在车上时控制器占有的是车，但弹药/血量都记在人身上，所以按车找司机回读。
	const APawn* Pawn = PlayerPawn();
	if (const AFpsPawn* Fps = Cast<AFpsPawn>(Pawn))
	{
		return Fps;
	}
	if (const ASimpleVehiclePawn* Vehicle = Cast<ASimpleVehiclePawn>(Pawn))
	{
		return Cast<AFpsPawn>(Vehicle->GetDriver());
	}
	return nullptr;
}

int32 UFpsHUDWidget::GetDisplayedHealth() const
{
	// 口径换算只在这里发生：组件里是 0..1，HUD 显示是 0..MaxHealthPlaceholder（100）。
	// 拿不到玩家/健康组件时回落占位满血，保持「暂时还没有扣血，就占位用」的原语义。
	if (const AFpsPawn* Fps = FpsPawnForDisplay())
	{
		const float Health01 = FMath::Clamp(Fps->GetHealth01(), 0.f, 1.f);
		return FMath::RoundToInt(Health01 * MaxHealthPlaceholder);
	}
	return FMath::RoundToInt(MaxHealthPlaceholder);
}

void UFpsHUDWidget::Refresh()
{
	// 左下角血量：有血量真源就显示 0..100，没有就保持原来的占位满血。
	SetTextSafe(txt_Health, FString::Printf(TEXT("%d"), GetDisplayedHealth()));

	// 右下角子弹/手雷：AFpsPawn 的两个 BlueprintReadWrite 计数。
	// 在车上时控制器占有的是车，但弹药仍记在人身上，所以按车找司机回读。
	const APawn* Pawn = PlayerPawn();
	const AFpsPawn* Fps = FpsPawnForDisplay();
	if (Fps)
	{
		SetTextSafe(txt_Ammo, FString::Printf(TEXT("%d"), Fps->Ammo));
		SetTextSafe(txt_Grenade, FString::Printf(TEXT("%d"), Fps->GrenadeCount));
	}
	else
	{
		SetTextSafe(txt_Ammo, TEXT("--"));
		SetTextSafe(txt_Grenade, TEXT("--"));
	}

	// 右中部载具三行：名字、上下车提示、时速。
	if (const ASimpleVehiclePawn* VehicleInUse = Cast<ASimpleVehiclePawn>(Pawn))
	{
		SetTextSafe(txt_VehicleName, VehicleInUse->GetVehicleDisplayName());
		SetTextSafe(txt_EnterPrompt, ExitPromptText);
		SetTextSafe(txt_VehicleSpeed, FString::Printf(TEXT("%.0f km/h"), GetDisplayedSpeedKmh()));
		return;
	}

	// 没上车：附近有车就把名字和「按 E 上车」摆出来，时速显示 0。
	const ASimpleVehiclePawn* Nearby = NearestVehicle();
	if (Nearby)
	{
		SetTextSafe(txt_VehicleName, Nearby->GetVehicleDisplayName());
		SetTextSafe(txt_EnterPrompt, EnterPromptText);
		SetTextSafe(txt_VehicleSpeed, TEXT("0 km/h"));
	}
	else
	{
		SetTextSafe(txt_VehicleName, TEXT(""));
		SetTextSafe(txt_EnterPrompt, TEXT(""));
		SetTextSafe(txt_VehicleSpeed, TEXT(""));
	}
}

void UFpsHUDWidget::SetTextSafe(TObjectPtr<UTextBlock>& Block, const FString& Text) const
{
	// BindWidgetOptional：WBP 里少摆一个控件也不崩，只是那一条不显示。
	if (Block)
	{
		Block->SetText(FText::FromString(Text));
	}
}
