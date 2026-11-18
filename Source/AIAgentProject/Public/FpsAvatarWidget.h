// Copyright FPS Agent Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "FpsAvatarWidget.generated.h"

class UTexture2D;

/**
 * Avatar frame for the FPS HUD.
 * Sits in the bottom-left corner, HealthBarSpacing (10 px by default) to the
 * left of the health bar. The portrait image is fully configurable on the
 * widget (designer / blueprint defaults) or at runtime via SetAvatarTexture.
 * Drop it into WBP_FpsHUD, anchor it left of the health bar with a 16 px horizontal gap, keep DesiredSize 192x48 and vertical Alignment (0.0, 0.5).
 * 10 px gap.
 */
UCLASS(meta = (DisplayName = "FPS Avatar Frame"))
class AIAGENTPROJECT_API UFpsAvatarFrameWidget : public UWidget
{
	GENERATED_BODY()

public:
	UFpsAvatarFrameWidget(const FObjectInitializer& ObjectInitializer);

	/** Portrait image shown inside the frame. Assign any texture in the designer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	UTexture2D* AvatarTexture = nullptr;

	/** Size of the portrait image, in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	FVector2D AvatarSize = FVector2D(64.f, 64.f);

	/** Frame thickness around the portrait. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	float FramePadding = 4.f;

	/** Frame colour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	FLinearColor FrameColor = FLinearColor(0.86f, 0.76f, 0.36f, 1.f);

	/** Horizontal gap kept between this frame and the health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	float HealthBarSpacing = 10.f;

	/** Designer-configurable portrait image source (texture asset) shown left of the health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Avatar")
	FSoftObjectPath PortraitImagePath;

	/** Tint applied to the portrait image. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Avatar")
	FLinearColor AvatarTint = FLinearColor::White;

	UFUNCTION(BlueprintCallable, Category = "Avatar")
	void SetAvatarTexture(UTexture2D* InTexture);

	UFUNCTION(BlueprintCallable, Category = "Avatar")
	void SetAvatarTint(FLinearColor InTint);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	TSharedPtr<class SImage> AvatarImage;
	FSlateBrush AvatarBrush;
};
