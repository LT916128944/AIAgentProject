// Copyright FPS Agent Project. All Rights Reserved.

#include "FpsAvatarWidget.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"

#define LOCTEXT_NAMESPACE "FpsAvatarFrameWidget"

UFpsAvatarFrameWidget::UFpsAvatarFrameWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AvatarBrush.DrawAs = ESlateBrushDrawType::Image;
	AvatarBrush.ImageSize = AvatarSize;
}

void UFpsAvatarFrameWidget::SetAvatarTexture(UTexture2D* InTexture)
{
	AvatarTexture = InTexture;
	AvatarBrush.SetResourceObject(InTexture);
	AvatarBrush.ImageSize = AvatarSize;

	if (AvatarImage.IsValid())
	{
		AvatarImage->SetImage(&AvatarBrush);
	}
}

void UFpsAvatarFrameWidget::SetAvatarTint(FLinearColor InTint)
{
	AvatarTint = InTint;

	if (AvatarImage.IsValid())
	{
		AvatarImage->SetColorAndOpacity(AvatarTint);
	}
}

TSharedRef<SWidget> UFpsAvatarFrameWidget::RebuildWidget()
{
	AvatarBrush.SetResourceObject(AvatarTexture);
	AvatarBrush.ImageSize = AvatarSize;

	const float OuterWidth = AvatarSize.X + FramePadding * 2.f;
	const float OuterHeight = AvatarSize.Y + FramePadding * 2.f;

	return SNew(SBox)
		.WidthOverride(OuterWidth)
		.HeightOverride(OuterHeight)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FrameColor)
			.Padding(FMargin(FramePadding))
			[
				SAssignNew(AvatarImage, SImage)
				.Image(&AvatarBrush)
				.ColorAndOpacity(AvatarTint)
			]
		];
}

void UFpsAvatarFrameWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	AvatarBrush.SetResourceObject(AvatarTexture);
	AvatarBrush.ImageSize = AvatarSize;

	if (AvatarImage.IsValid())
	{
		AvatarImage->SetImage(&AvatarBrush);
		AvatarImage->SetColorAndOpacity(AvatarTint);
	}
}

void UFpsAvatarFrameWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	AvatarImage.Reset();
}

#if WITH_EDITOR
const FText UFpsAvatarFrameWidget::GetPaletteCategory()
{
	return LOCTEXT("AvatarPalette", "FPS HUD");
}
#endif

#undef LOCTEXT_NAMESPACE
