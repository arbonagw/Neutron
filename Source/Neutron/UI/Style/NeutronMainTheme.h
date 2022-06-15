﻿// Neutron - Gwennaël Arbona

#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "NeutronMainTheme.generated.h"

USTRUCT()
struct NEUTRON_API FNeutronMainTheme : public FSlateWidgetStyle
{
	GENERATED_BODY()

	FNeutronMainTheme()
		: GenericMenuWidth(700)
		, GenericMenuHeight(600)
		, BlurRadius(0)
		, BlurStrength(0)
		, PositiveColor(FLinearColor::Blue)
		, ContrastingColor(FLinearColor::Yellow)
		, NegativeColor(FLinearColor::Red)
		, NeutralColor(FLinearColor::Gray)
	{}

	/*----------------------------------------------------
	    Interface
	----------------------------------------------------*/

	static const FName TypeName;
	const FName        GetTypeName() const override
	{
		return TypeName;
	}

	static const FNeutronMainTheme& GetDefault()
	{
		static FNeutronMainTheme Default;
		return Default;
	}

	void GetResources(TArray<const FSlateBrush*>& OutBrushes) const
	{
		OutBrushes.Add(&MainMenuBackground);
		OutBrushes.Add(&MainMenuGenericBackground);
		OutBrushes.Add(&MainMenuDarkBackground);
		OutBrushes.Add(&MainMenuGenericBorder);
		OutBrushes.Add(&MainMenuManipulator);

		OutBrushes.Add(&TableHeaderBackground);
		OutBrushes.Add(&TableOddBackground);
		OutBrushes.Add(&TableEvenBackground);

		OutBrushes.Add(&Black);
		OutBrushes.Add(&White);
	}

	/*----------------------------------------------------
	    Data
	----------------------------------------------------*/

	// General
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin ContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) FMargin VerticalContentPadding;
	UPROPERTY(EditDefaultsOnly, Category = General) int32 GenericMenuWidth;
	UPROPERTY(EditDefaultsOnly, Category = General) int32 GenericMenuHeight;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurRadius;
	UPROPERTY(EditDefaultsOnly, Category = General) float BlurStrength;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor PositiveColor;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor ContrastingColor;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor NegativeColor;
	UPROPERTY(EditDefaultsOnly, Category = General) FLinearColor NeutralColor;

	// Fonts
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle NotificationFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle HeadingFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle MainFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle InfoFont;
	UPROPERTY(EditDefaultsOnly, Category = Fonts) FTextBlockStyle KeyFont;

	// Main
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuPatternedBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuDarkBackground;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuGenericBorder;
	UPROPERTY(EditDefaultsOnly, Category = Brushes) FSlateBrush MainMenuManipulator;

	// Tables
	UPROPERTY(EditDefaultsOnly, Category = Tables) FSlateBrush TableHeaderBackground;
	UPROPERTY(EditDefaultsOnly, Category = Tables) FSlateBrush TableOddBackground;
	UPROPERTY(EditDefaultsOnly, Category = Tables) FSlateBrush TableEvenBackground;

	// Basic
	UPROPERTY(EditDefaultsOnly, Category = Basic) FSlateBrush Black;
	UPROPERTY(EditDefaultsOnly, Category = Basic) FSlateBrush White;

	// Widget styles
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FSliderStyle SliderStyle;
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FScrollBoxStyle ScrollBoxStyle;
	UPROPERTY(EditDefaultsOnly, Category = Widgets) FProgressBarStyle ProgressBarStyle;

	// Sound
	UPROPERTY(EditDefaultsOnly, Category = Sound) FSlateSound ConfirmSound;
	UPROPERTY(EditDefaultsOnly, Category = Sound) FSlateSound CancelSound;
	UPROPERTY(EditDefaultsOnly, Category = Sound) FSlateSound DeleteSound;
	UPROPERTY(EditDefaultsOnly, Category = Sound) FSlateSound NotifySound;
};

/*----------------------------------------------------
    Wrapper class
----------------------------------------------------*/

UCLASS()
class UNeutronMainThemeContainer : public USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const override
	{
		return static_cast<const struct FSlateWidgetStyle*>(&Style);
	}

	UPROPERTY(Category = Neutron, EditDefaultsOnly, meta = (ShowOnlyInnerProperties))
	FNeutronMainTheme Style;
};
