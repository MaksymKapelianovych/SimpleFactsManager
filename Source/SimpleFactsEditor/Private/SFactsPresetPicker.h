// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class SIMPLEFACTSEDITOR_API SFactsPresetPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SFactsPresetPicker ) {}

		// SLATE_EVENT( FOnSelectionChanged, PresetSelected )
	SLATE_END_ARGS()


	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

private:
	TSharedRef<SWidget> HandleGeneratePresetWidget( TSharedPtr<FAssetData> AssetData );
	void HandleSelectionChanged( TSharedPtr<FAssetData> AssetData, ESelectInfo::Type Arg );

	
private:
	TSharedPtr<SComboBox<TSharedPtr<FAssetData>>> PresetPicker;
	TArray< TSharedPtr< FAssetData > > PresetAssets;
};
