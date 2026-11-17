#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FpsHealthBarWidget.generated.h"

UCLASS()
class AIAGENTPROJECT_API UFpsHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFpsHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Health")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	float LowHealthRatio = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FLinearColor FillColor = FLinearColor(0.10f, 0.75f, 0.20f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FLinearColor LowHealthColor = FLinearColor(0.90f, 0.12f, 0.06f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FLinearColor BackColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FLinearColor BorderColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FVector2D BarSize = FVector2D(260.0f, 26.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	FVector2D BarOffset = FVector2D(48.0f, -96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Health")
	float BorderThickness = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void SetHealth(float InCurrent);

	UFUNCTION(BlueprintCallable, Category = "FPS|Health")
	void SetHealthRatio(float InRatio);

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	float GetHealthRatio() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "FPS|Health")
	bool IsLowHealth() const;

protected:
	virtual void NativeConstruct() override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void ApplyBottomLeftAnchoring();
};
