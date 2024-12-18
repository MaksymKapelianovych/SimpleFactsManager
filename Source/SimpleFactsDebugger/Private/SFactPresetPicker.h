// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UFactPreset;

class SFactPresetPicker : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam( FOnSelectionChanged, const UFactPreset* )
	
	SLATE_BEGIN_ARGS( SFactPresetPicker ) {}

		SLATE_EVENT( FOnSelectionChanged, OnPresetSelected )
	SLATE_END_ARGS()


	void Construct( const FArguments& InArgs, const TArray< FAssetData >& PresetsData );

private:
	void CachePresetsData(const TArray< FAssetData >& PresetsData);
	
	// List view
	TSharedRef< ITableRow > HandleGeneratePresetWidget( TSharedPtr< FAssetData > AssetData, const TSharedRef< STableViewBase >& OwnerTable );
	void HandleSelectionChanged( TSharedPtr< FAssetData > AssetData, ESelectInfo::Type Type );
	void HandleSortListView( EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode );
	EColumnSortMode::Type GetColumnSortMode() const;

	// Focus
	EActiveTimerReturnType SetFocusPostConstruct( double InCurrentTime, float InDeltaTime ) const;

	// Search
	void HandleSearchTextChanged( const FText& Text );
	void HandleSearchTextCommitted( const FText& Text, ETextCommit::Type Type );
	FReply HandleKeyDownFromSearchBox( const FGeometry& Geometry, const FKeyEvent& KeyEvent );

	void AdjustActiveSelection( int32 SelectionDelta );
	
private:
	TSharedPtr< SListView< TSharedPtr< FAssetData > > > PresetsListView;
	TArray< TSharedPtr< FAssetData > > AllPresetAssets;
	TArray< TSharedPtr< FAssetData > > FilteredPresetAssets;
	
	TSharedPtr< SSearchBox > SearchBox;

	FOnSelectionChanged OnPresetSelected;

	EColumnSortMode::Type CurrentSortMode = EColumnSortMode::Ascending;
};
