// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsPresetPicker.h"
#include "FactsDebuggerStyle.h"
#include "FactsPreset.h"
#include "SlateOptMacros.h"
#include "Logging/StructuredLog.h"
#include "Widgets/Input/SSearchBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactsPresetPicker::Construct( const FArguments& InArgs, const TArray< FAssetData >& PresetsData  )
{
	OnPresetSelected = InArgs._OnPresetSelected;
	
	CachePresetsData( PresetsData );
	
	ChildSlot
	[
		SNew ( SVerticalBox )

		+ SVerticalBox::Slot()
		.HAlign( HAlign_Fill )
		.VAlign( VAlign_Top )
		.Padding( 4.f, 2.f )
		.AutoHeight()
		[
			SAssignNew ( SearchBox, SSearchBox )
			.OnTextChanged( this, &SFactsPresetPicker::HandleSearchTextChanged )
			.OnTextCommitted( this, &SFactsPresetPicker::HandleSearchTextCommitted )
			.OnKeyDownHandler( this, &SFactsPresetPicker::HandleKeyDownFromSearchBox )
		]

		+ SVerticalBox::Slot()
		[
			SNew( SBorder )
			.Padding( 6.f )
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			[
				SAssignNew( PresetPicker, SListView< TSharedPtr< FAssetData > > )
				.SelectionMode( ESelectionMode::Type::Single )
				.ListItemsSource( &FilteredPresetAssets )
				.OnGenerateRow( this, &SFactsPresetPicker::HandleGeneratePresetWidget )
				.OnSelectionChanged( this, &SFactsPresetPicker::HandleSelectionChanged )
				.HeaderRow
				(
					SNew( SHeaderRow )
					+ SHeaderRow::Column( "Name" ).DefaultLabel( NSLOCTEXT( "FactsDebugger", "ProfilerListColName", "Name" ) )
					.SortPriority( EColumnSortPriority::Primary )
					.SortMode( this, &SFactsPresetPicker::GetColumnSortMode )
					.OnSort( this, &SFactsPresetPicker::HandleSortListView )
				)
			]
		]
	];

	HandleSearchTextChanged( FText::GetEmpty() );
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedPtr<SWidget> SFactsPresetPicker::GetWidgetToFocusOnOpen()
{
	return SearchBox;
}

void SFactsPresetPicker::CachePresetsData(const TArray<FAssetData>& PresetsData)
{
	AllPresetAssets.Reserve( PresetsData.Num() );
	for ( const FAssetData& AssetData : PresetsData )
	{
		AllPresetAssets.Add( MakeShared< FAssetData >( AssetData ) );
	}

	AllPresetAssets.Sort( []( const TSharedPtr< FAssetData >& Lhs, const TSharedPtr< FAssetData >& Rhs )
	{
		return Lhs->AssetName.Compare( Rhs->AssetName ) < 0;
	} );
}

TSharedRef< ITableRow > SFactsPresetPicker::HandleGeneratePresetWidget( TSharedPtr< FAssetData > AssetData, const TSharedRef< STableViewBase >& OwnerTable )
{
	if ( !ensure( AssetData.IsValid() ) )
	{
		return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable );
	}

	return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable )
		.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>( "ContentBrowser.AssetListView.ColumnListTableRow" ) )
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.f, 0.f, 0.f, 0.f )
			[
				SNew( SOverlay )

				+ SOverlay::Slot()
				.VAlign( VAlign_Top )
				.HAlign( HAlign_Center )
				[
					SNew( SBorder )
					.BorderImage( FAppStyle::GetBrush( "AssetThumbnail.AssetBackground" ) )
					[
						SNew( SImage )
						.Image( FFactsDebuggerStyle::Get().GetBrush( "ClassThumbnail.FactsPreset" ) )
						.DesiredSizeOverride( FVector2d{ 36.f } )
					]
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
			.Padding( 2.f, 0.f, 0.f, 0.f )
			[
				SNew( SVerticalBox )

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.f, 1.f )
				[
					SNew( STextBlock )
					.Text( FText::FromName( AssetData->AssetName ) )
					.Font( FAppStyle::Get().GetFontStyle("ContentBrowser.AssetTileViewNameFont") )
					.HighlightText( SearchBox.Get(), &SSearchBox::GetText )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.f, 1.f )
				[
					SNew( STextBlock )
					.Text( FText::FromName( AssetData->PackagePath ) )
					.Font( FAppStyle::Get().GetFontStyle("ContentBrowser.AssetListViewClassFont") )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
			]
		];
}

void SFactsPresetPicker::HandleSelectionChanged( TSharedPtr<FAssetData> AssetData, ESelectInfo::Type Type )
{
	if ( Type == ESelectInfo::Type::Direct || Type == ESelectInfo::Type::OnNavigation )
	{
		return;
	}

	if ( OnPresetSelected.IsBound() )
	{
		OnPresetSelected.Execute( Cast< UFactsPreset >( AssetData->GetAsset() ) );
	}
}

void SFactsPresetPicker::HandleSortListView(EColumnSortPriority::Type SortPriority, const FName& ColumnName,
	EColumnSortMode::Type SortMode)
{
	CurrentSortMode = SortMode;
	
	if ( ColumnName == "Name" )
	{
		UE_LOGFMT( LogTemp, Warning, "SortPriority - {0}, SortMode - {1}", SortPriority,  SortMode );
		AllPresetAssets.Sort( [ SortMode ]( const TSharedPtr< FAssetData >& Lhs, const TSharedPtr< FAssetData >& Rhs )
		{
			int32 CompareResult = Lhs->AssetName.Compare( Rhs->AssetName );
			return SortMode == EColumnSortMode::Ascending ? CompareResult < 0 : CompareResult > 0;
		} );
		
		FilteredPresetAssets.Sort( [ SortMode ]( const TSharedPtr< FAssetData >& Lhs, const TSharedPtr< FAssetData >& Rhs )
        {
        	int32 CompareResult = Lhs->AssetName.Compare( Rhs->AssetName );
        	return SortMode == EColumnSortMode::Ascending ? CompareResult < 0 : CompareResult > 0;
        } );
	}

	PresetPicker->RequestListRefresh();
}

EColumnSortMode::Type SFactsPresetPicker::GetColumnSortMode() const
{
	return CurrentSortMode;
}

void SFactsPresetPicker::HandleSearchTextChanged( const FText& Text )
{
	ON_SCOPE_EXIT{ PresetPicker->RequestListRefresh(); };

	FilteredPresetAssets.Empty();
	if ( Text.IsEmpty() )
	{
		FilteredPresetAssets = AllPresetAssets;
		return;
	}

	
	const FString& FilterString = Text.ToString();

	for ( const TSharedPtr< FAssetData >& AssetData : AllPresetAssets )
	{
		if ( AssetData->AssetName.ToString().Contains( FilterString ) )
		{
			FilteredPresetAssets.Add( AssetData );
		}
	}
}

void SFactsPresetPicker::HandleSearchTextCommitted( const FText& Text, ETextCommit::Type Type )
{
	HandleSearchTextChanged( Text );

	if ( Type == ETextCommit::Type::OnEnter )
	{
		TArray< TSharedPtr< FAssetData > > SelectionSet = PresetPicker->GetSelectedItems();
		if ( SelectionSet.Num() == 0 )
		{
			AdjustActiveSelection( 1 );
			SelectionSet = PresetPicker->GetSelectedItems();
		}
		
		if ( OnPresetSelected.IsBound() )
		{
			OnPresetSelected.Execute( Cast< UFactsPreset >( SelectionSet[ 0 ]->GetAsset() ) );
		}
	}
}

FReply SFactsPresetPicker::HandleKeyDownFromSearchBox( const FGeometry& Geometry, const FKeyEvent& KeyEvent )
{
	int32 SelectionDelta = 0;

	if ( KeyEvent.GetKey() == EKeys::Up )
	{
		SelectionDelta = -1;
	}
	else if ( KeyEvent.GetKey() == EKeys::Down )
	{
		SelectionDelta = +1;
	}

	if ( SelectionDelta != 0 )
	{
		AdjustActiveSelection( SelectionDelta );
		
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SFactsPresetPicker::AdjustActiveSelection(int32 SelectionDelta)
{
	TArray< TSharedPtr< FAssetData > > SelectionSet = PresetPicker->GetSelectedItems();
	int32 SelectedSuggestion = INDEX_NONE;

	if ( SelectionSet.Num() > 0 )
	{
		if ( FilteredPresetAssets.Find( SelectionSet[ 0 ], /*out*/ SelectedSuggestion ) == false )
		{
			// Should never happen
			ensureMsgf( false, TEXT( "SFactsPresetPicker has a selected item that wasn't in the filtered list" ) );
			return;
		}
	}
	else
	{
		SelectedSuggestion = 0;
		SelectionDelta = 0;
	}

	if ( FilteredPresetAssets.Num() > 0 )
	{
		// Move up or down one, wrapping around
		SelectedSuggestion = ( SelectedSuggestion + SelectionDelta + FilteredPresetAssets.Num() ) % FilteredPresetAssets.Num();

		// Pick the new asset
		const TSharedPtr< FAssetData >& NewSelection = FilteredPresetAssets[ SelectedSuggestion ];

		PresetPicker->RequestScrollIntoView( NewSelection );
		PresetPicker->SetSelection( NewSelection );
	}
	else
	{
		PresetPicker->ClearSelection();
	}
}
