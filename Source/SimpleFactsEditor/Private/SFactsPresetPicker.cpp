// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsPresetPicker.h"
#include "FactsEditorStyle.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SSearchBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SFactsPresetPicker::Construct( const FArguments& InArgs, const TArray< FAssetData >& PresetsData  )
{
	PresetAssets.Reserve( PresetsData.Num() );
	for ( const FAssetData& AssetData : PresetsData )
	{
		PresetAssets.Add( MakeShared< FAssetData >( AssetData ) );
	}
	
	ChildSlot
	[
		SNew ( SVerticalBox )

		+ SVerticalBox::Slot()
		.HAlign( HAlign_Fill )
		.VAlign( VAlign_Top )
		.Padding( 4.f, 2.f )
		.AutoHeight()
		[
			SNew ( SSearchBox )
		]

		+ SVerticalBox::Slot()
		[
			SAssignNew( PresetPicker, SListView< TSharedPtr< FAssetData > > )
			.ListItemsSource( &PresetAssets )
			.OnGenerateRow( this, &SFactsPresetPicker::HandleGeneratePresetWidget )
			.OnSelectionChanged( this, &SFactsPresetPicker::HandleSelectionChanged )
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedRef< ITableRow > SFactsPresetPicker::HandleGeneratePresetWidget( TSharedPtr< FAssetData > AssetData, const TSharedRef< STableViewBase >& OwnerTable )
{
	if ( !ensure( AssetData.IsValid() ) )
	{
		return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable );
	}

	return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable )
		.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.AlternatingRow") )
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SOverlay )

				+ SOverlay::Slot()
				.VAlign( VAlign_Bottom )
				.HAlign( HAlign_Right )
				[
					SNew( SImage )
					.Image( FFactsEditorStyleStyle::Get().GetBrush( "ClassThumbnail.FactsPreset" ) )
					.DesiredSizeOverride( FVector2d{ 36.f } )
				]
				
				+ SOverlay::Slot()
				.VAlign( VAlign_Bottom )
				.HAlign( HAlign_Fill )
				[
					SNew( SBorder )
					.BorderImage( FAppStyle::GetBrush( "WhiteBrush" ) )
					.BorderBackgroundColor( FLinearColor( FColor( 243, 113, 42 ) ) ) // todo: move to StyleSet
					.Padding( FMargin{ 0.f, 2.f, 0.f, 0.f } )
				]
				
			]

			+ SHorizontalBox::Slot()
			.FillWidth( 1.f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromName( AssetData->AssetName ) )
			]
		];
}

void SFactsPresetPicker::HandleSelectionChanged( TSharedPtr<FAssetData> AssetData, ESelectInfo::Type Arg )
{
	
}
