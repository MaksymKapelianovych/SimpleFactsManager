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
	void Construct( const FArguments& InArgs, const TArray< FAssetData >& PresetsData );

	/** @returns widget to focus (search box) when the picker is opened. */
	TSharedPtr<SWidget> GetWidgetToFocusOnOpen();

private:
	// List view
	TSharedRef< ITableRow > HandleGeneratePresetWidget( TSharedPtr< FAssetData > AssetData, const TSharedRef< STableViewBase >& OwnerTable );
	void HandleSelectionChanged( TSharedPtr<FAssetData> AssetData, ESelectInfo::Type Arg );

	// Search
	void HandleSearchTextChanged( const FText& Text );
	
private:
	TSharedPtr< SListView< TSharedPtr< FAssetData > > > PresetPicker;
	TArray< TSharedPtr< FAssetData > > AllPresetAssets;
	TArray< TSharedPtr< FAssetData > > FilteredPresetAssets;
	
	TSharedPtr<SSearchBox> SearchBox;
};
