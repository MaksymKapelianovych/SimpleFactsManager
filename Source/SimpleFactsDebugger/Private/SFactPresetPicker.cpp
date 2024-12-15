// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactPresetPicker.h"
#include "FactDebuggerStyle.h"
#include "FactPreset.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SSearchBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactPresetPicker::Construct( const FArguments& InArgs, const TArray< FAssetData >& PresetsData  )
{
	OnPresetSelected = InArgs._OnPresetSelected;
	
	CachePresetsData( PresetsData );
	
	ChildSlot
	[
		SNew ( SVerticalBox )

		// -------------------------------------------------------------------------------------------------------------
		// Search box
		+ SVerticalBox::Slot()
		.HAlign( HAlign_Fill )
		.VAlign( VAlign_Top )
		.Padding( 4.f, 2.f )
		.AutoHeight()
		[
			SAssignNew ( SearchBox, SSearchBox )
			.OnTextChanged( this, &SFactPresetPicker::HandleSearchTextChanged )
			.OnTextCommitted( this, &SFactPresetPicker::HandleSearchTextCommitted )
			.OnKeyDownHandler( this, &SFactPresetPicker::HandleKeyDownFromSearchBox )
		]

		// -------------------------------------------------------------------------------------------------------------
		// Presets list
		+ SVerticalBox::Slot()
		[
			SNew( SBorder )
			.Padding( 6.f )
			.BorderImage( FAppStyle::GetBrush( "Brushes.Panel" ) )
			[
				SAssignNew( PresetsListView, SListView< TSharedPtr< FAssetData > > )
				.SelectionMode( ESelectionMode::Type::Single )
				.ListItemsSource( &FilteredPresetAssets )
				.OnGenerateRow( this, &SFactPresetPicker::HandleGeneratePresetWidget )
				.OnSelectionChanged( this, &SFactPresetPicker::HandleSelectionChanged )
				.HeaderRow
				(
					SNew( SHeaderRow )
					+ SHeaderRow::Column( "Name" )
					.DefaultLabel( NSLOCTEXT( "FactDebugger", "ProfilerListColName", "Name" ) )
					.SortPriority( EColumnSortPriority::Primary )
					.SortMode( this, &SFactPresetPicker::GetColumnSortMode )
					.OnSort( this, &SFactPresetPicker::HandleSortListView )
				)
			]
		]
	];

	HandleSearchTextChanged( FText::GetEmpty() );
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedPtr<SWidget> SFactPresetPicker::GetWidgetToFocusOnOpen()
{
	return SearchBox;
}

void SFactPresetPicker::CachePresetsData( const TArray< FAssetData >& PresetsData )
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

TSharedRef< ITableRow > SFactPresetPicker::HandleGeneratePresetWidget( TSharedPtr< FAssetData > AssetData, const TSharedRef< STableViewBase >& OwnerTable )
{
	if ( !ensure( AssetData.IsValid() ) )
	{
		return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable );
	}

	return SNew( STableRow< TSharedPtr< FAssetData > >, OwnerTable )
		.Style( FAppStyle::Get(), "TableView.AlternatingRow" )
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
					.BorderImage( FAppStyle::GetBrush( "Brushes.Recessed" ) )
					[
						SNew( SImage )
						.Image( FFactDebuggerStyle::Get().GetBrush( "ClassThumbnail.FactPreset" ) )
						.DesiredSizeOverride( FVector2d{ 36.f } )
					]
				]
				
				+ SOverlay::Slot()
				.VAlign( VAlign_Bottom )
				.HAlign( HAlign_Fill )
				[
					SNew( SBorder )
					.BorderImage( FAppStyle::GetBrush( "WhiteBrush" ) )
					.BorderBackgroundColor( FFactDebuggerStyle::Get().GetColor( "Colors.FactPreset" ) )
					.Padding( 0.f, 2.f, 0.f, 0.f )
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
					.Font( FFactDebuggerStyle::Get().GetFontStyle( "NameFont" ) )
					.HighlightText( SearchBox.Get(), &SSearchBox::GetText )
				]
				
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 0.f, 1.f )
				[
					SNew( STextBlock )
					.Text( FText::FromName( AssetData->PackagePath ) )
					.Font( FFactDebuggerStyle::Get().GetFontStyle( "PathFont" ) )
				]
			]
		];
}

void SFactPresetPicker::HandleSelectionChanged( TSharedPtr< FAssetData > AssetData, ESelectInfo::Type Type )
{
	if ( Type == ESelectInfo::Type::Direct || Type == ESelectInfo::Type::OnNavigation )
	{
		return;
	}

	if ( OnPresetSelected.IsBound() )
	{
		OnPresetSelected.Execute( Cast< UFactPreset >( AssetData->GetAsset() ) );
	}
}

void SFactPresetPicker::HandleSortListView( EColumnSortPriority::Type SortPriority, const FName& ColumnName, EColumnSortMode::Type SortMode )
{
	CurrentSortMode = SortMode;
	
	if ( ColumnName == "Name" )
	{
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

	PresetsListView->RequestListRefresh();
}

EColumnSortMode::Type SFactPresetPicker::GetColumnSortMode() const
{
	return CurrentSortMode;
}

void SFactPresetPicker::HandleSearchTextChanged( const FText& Text )
{
	ON_SCOPE_EXIT{ PresetsListView->RequestListRefresh(); };

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

void SFactPresetPicker::HandleSearchTextCommitted( const FText& Text, ETextCommit::Type Type )
{
	HandleSearchTextChanged( Text );

	if ( Type == ETextCommit::Type::OnEnter )
	{
		TArray< TSharedPtr< FAssetData > > SelectionSet = PresetsListView->GetSelectedItems();
		if ( SelectionSet.Num() == 0 )
		{
			AdjustActiveSelection( 1 );
			SelectionSet = PresetsListView->GetSelectedItems();
		}
		
		if ( OnPresetSelected.IsBound() )
		{
			OnPresetSelected.Execute( Cast< UFactPreset >( SelectionSet[ 0 ]->GetAsset() ) );
		}
	}
}

FReply SFactPresetPicker::HandleKeyDownFromSearchBox( const FGeometry& Geometry, const FKeyEvent& KeyEvent )
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

void SFactPresetPicker::AdjustActiveSelection(int32 SelectionDelta)
{
	TArray< TSharedPtr< FAssetData > > SelectionSet = PresetsListView->GetSelectedItems();
	int32 SelectedSuggestion = INDEX_NONE;

	if ( SelectionSet.Num() > 0 )
	{
		if ( FilteredPresetAssets.Find( SelectionSet[ 0 ], /*out*/ SelectedSuggestion ) == false )
		{
			// Should never happen
			ensureMsgf( false, TEXT( "SFactPresetPicker has a selected item that wasn't in the filtered list" ) );
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

		PresetsListView->RequestScrollIntoView( NewSelection );
		PresetsListView->SetSelection( NewSelection );
	}
	else
	{
		PresetsListView->ClearSelection();
	}
}
