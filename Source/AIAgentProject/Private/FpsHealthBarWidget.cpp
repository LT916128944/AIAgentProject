#include "FpsHealthBarWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Engine/World.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

UFpsHealthBarWidget::UFpsHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxHealth = 100.0f;
	CurrentHealth = 100.0f;
	LowHealthRatio = 0.3f;
	BarSize = FVector2D(260.0f, 26.0f);
	BarOffset = FVector2D(48.0f, -96.0f);
	BorderThickness = 2.0f;
}

void UFpsHealthBarWidget::SetHealth(float InCurrent)
{
	MaxHealth = 100.0f;
	CurrentHealth = FMath::Clamp(InCurrent, 0.0f, MaxHealth);
}

void UFpsHealthBarWidget::SetHealthRatio(float InRatio)
{
	MaxHealth = 100.0f;
	CurrentHealth = FMath::Clamp(InRatio, 0.0f, 1.0f) * MaxHealth;
}

float UFpsHealthBarWidget::GetHealthRatio() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

float UFpsHealthBarWidget::GetMaxHealth() const
{
	return MaxHealth;
}

bool UFpsHealthBarWidget::IsLowHealth() const
{
	return GetHealthRatio() <= LowHealthRatio;
}

void UFpsHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	MaxHealth = 100.0f;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	ApplyBottomLeftAnchoring();
}

void UFpsHealthBarWidget::ApplyBottomLeftAnchoring()
{
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
	if (CanvasSlot == nullptr)
	{
		return;
	}

	CanvasSlot->SetAutoSize(false);
	CanvasSlot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
	CanvasSlot->SetAlignment(FVector2D(0.0f, 1.0f));
	CanvasSlot->SetPosition(BarOffset);
	CanvasSlot->SetSize(BarSize);
	CanvasSlot->SetZOrder(10);
}

int32 UFpsHealthBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Size = (LocalSize.X > 1.0f && LocalSize.Y > 1.0f) ? LocalSize : BarSize;

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	if (Brush == nullptr)
	{
		return BaseLayer;
	}

	const float Border = FMath::Max(BorderThickness, 0.0f);
	const FVector2D InnerOrigin(Border, Border);
	const FVector2D InnerSize(FMath::Max(Size.X - Border * 2.0f, 0.0f), FMath::Max(Size.Y - Border * 2.0f, 0.0f));

	const float Ratio = GetHealthRatio();
	const bool bLow = IsLowHealth();

	float Pulse = 1.0f;
	if (bLow)
	{
		const UWorld* World = GetWorld();
		const float TimeSeconds = (World != nullptr) ? World->GetTimeSeconds() : 0.0f;
		Pulse = 0.55f + 0.45f * FMath::Abs(FMath::Sin(TimeSeconds * 6.0f));
	}

	int32 Layer = BaseLayer + 1;

	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2D::ZeroVector)), Brush, ESlateDrawEffect::None, BorderColor);
	Layer++;

	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(InnerSize, FSlateLayoutTransform(InnerOrigin)), Brush, ESlateDrawEffect::None, BackColor);
	Layer++;

	const float FillWidth = InnerSize.X * Ratio;
	if (FillWidth > 0.5f)
	{
		FLinearColor Fill = FMath::Lerp(FillColor, LowHealthColor, bLow ? Pulse : 0.0f);
		Fill.A = 1.0f;
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2D(FillWidth, InnerSize.Y), FSlateLayoutTransform(InnerOrigin)), Brush, ESlateDrawEffect::None, Fill);
		Layer++;
	}

	if (bLow)
	{
		FLinearColor Warn = LowHealthColor;
		Warn.A = 0.25f + 0.45f * Pulse;
		const FVector2D WarnOrigin(Border * 2.0f, Border * 2.0f);
		const FVector2D WarnSize(FMath::Max(InnerSize.X - Border * 2.0f, 0.0f), FMath::Max(InnerSize.Y - Border * 2.0f, 0.0f));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(WarnSize, FSlateLayoutTransform(WarnOrigin)), Brush, ESlateDrawEffect::None, Warn);
		Layer++;
	}

	return Layer;
}
