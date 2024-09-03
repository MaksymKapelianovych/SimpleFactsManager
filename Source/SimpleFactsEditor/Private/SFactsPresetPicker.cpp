// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsPresetPicker.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SFactsPresetPicker::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SAssignNew( PresetPicker, SComboBox< TSharedPtr< FAssetData > > )
		.OptionsSource( &PresetAssets )
		.OnGenerateWidget( this, &SFactsPresetPicker::HandleGeneratePresetWidget )
		.OnSelectionChanged( this, &SFactsPresetPicker::HandleSelectionChanged )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::GetBrush( "NoBorder" ) )
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedRef<SWidget> SFactsPresetPicker::HandleGeneratePresetWidget( TSharedPtr<FAssetData> AssetData )
{
	return SNullWidget::NullWidget;
}

void SFactsPresetPicker::HandleSelectionChanged( TSharedPtr<FAssetData> AssetData, ESelectInfo::Type Arg )
{
	
}
