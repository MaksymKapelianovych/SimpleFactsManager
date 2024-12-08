// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsDebugger.h"

#include "ContentBrowserModule.h"
#include "FactsDebuggerSettingsLocal.h"
#include "FactsDebuggerStyle.h"
#include "FactsPreset.h"
#include "FactSubsystem.h"
#include "IDocumentation.h"
#include "SFactsSearchToggle.h"
#include "SFactsExpanderArrow.h"
#include "SFactsPresetPicker.h"
#include "SimpleFactsDebugger.h"
#include "SlateOptMacros.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "FactsDebugger"

TSet< FFactTag > SFactsDebugger::MainExpandedFacts;
TSet< FFactTag > SFactsDebugger::FavoritesExpandedFacts;
TArray< FFactTag > SFactsDebugger::FavoriteFacts;

#define TOGGLE_FACT_SETTING( PropertyName ) \
	[ this ]() { \
		UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >(); \
		Settings->PropertyName = !Settings->PropertyName; \
		Settings->SaveConfig(); \
		 \
		RebuildFactTreeItems(); \
	}


namespace Utils
{
	int32 AllFilteredFactsCount = 0;
	int32 AllFavoriteFactsCount = 0;
	
	int32 CurrentFilteredFactsCount = 0;
	int32 CurrentFavoriteFactsCount = 0;
	
	struct FFilterOptions
	{
		TArray< FString > SearchToggleStrings;
		TArray< FString > SearchBarStrings;
		bool bFilteringFavorites;
		
	};
	void FilterFactItemChildren( TArray< FFactTreeItemPtr>& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );

	void CopyMatchedItem( FFactTreeItemPtr SourceItem, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options, bool bShouldFilterChildren )
	{
		if ( bShouldFilterChildren )
		{
			TArray< FFactTreeItemPtr > FilteredChildren;
			FilterFactItemChildren( SourceItem->Children, FilteredChildren, Options );
			if ( FilteredChildren.Num() )
			{
				FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
				*NewItem = *SourceItem;
				NewItem->InitItem();
				NewItem->Children = FilteredChildren;
			}
		}
		else
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
			NewItem->InitItem();			}
	}

	bool MatchSearchToggle( const TArray< FString >& SearchStrings, const FString& TagString )
	{
		if ( SearchStrings.IsEmpty() )
		{
			return true;
		}
		
		for ( const FString& SearchString : SearchStrings )
		{
			TArray< FString > Tokens;
			SearchString.ParseIntoArray( Tokens, TEXT( " " ) );

			auto Projection = [ TagString ]( const FString& Token ) { return TagString.Contains( Token ); };
			if ( Algo::AllOf( Tokens, Projection ) )
			{
				return true;
			}
		}

		return false;
	}

	bool MatchSearchBox( const TArray< FString >& SearchStrings, const FString& TagString )
	{
		if ( SearchStrings.IsEmpty() )
		{
			return true;
		}
		
		for ( const FString& SearchString : SearchStrings )
		{
			if ( TagString.Contains( SearchString ) == false )
			{
				return false;
			}
		}

		return true;
	}

	enum class ETagMatchResult
	{
		None,
		Partial,
		Full
	};
		
	ETagMatchResult MatchFavorites( FFactTag CheckedTag )
	{
		bool bPartialMatch = false;
		for ( FFactTag FavoriteFact : SFactsDebugger::FavoriteFacts )
		{
			if ( CheckedTag == FavoriteFact )
			{
				return ETagMatchResult::Full;
			}

			if ( CheckedTag.MatchesTag( FavoriteFact ) || FavoriteFact.MatchesTag( CheckedTag ) )
			{
				bPartialMatch = true;
			}
		}

		if ( bPartialMatch )
		{
			return ETagMatchResult::Partial;
		}

		return ETagMatchResult::None;
	};
	
	void FilterFactItemChildren( TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		const UFactsDebuggerSettingsLocal* Settings = GetDefault< UFactsDebuggerSettingsLocal >();
		auto MatchText = [ &Options ]( const FString& TagString )
		{
			return MatchSearchBox( Options.SearchBarStrings, TagString ) && MatchSearchToggle( Options.SearchToggleStrings, TagString );
		};

		for ( const FFactTreeItemPtr& SourceItem : SourceArray )
		{
			ETagMatchResult Result = MatchFavorites( SourceItem->Tag );

			switch ( Result ) {
			case ETagMatchResult::None: // Fact and it's parent tags is not in favorites
				{
					if ( Options.bFilteringFavorites == false )
					{
						bool bShouldFilterChildren = MatchText( SourceItem->Tag.ToString() ) == false;
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, bShouldFilterChildren );
					}
				}
				break;
			case ETagMatchResult::Partial: // Fact is not in favorites, but one of the parent/children tags is 
				{
					if ( Options.bFilteringFavorites )
					{
						bool bShouldFilterChildren = ( Options.SearchBarStrings.IsEmpty() && Options.SearchToggleStrings.IsEmpty() ) || MatchText( SourceItem->Tag.ToString() ) == false;
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, bShouldFilterChildren );

					}
					else
					{
						bool bShouldFilterChildren = Settings->bRemoveFavoritesFromMainTree || MatchText( SourceItem->Tag.ToString() ) == false;
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, bShouldFilterChildren );
					}
				}
				break;
			case ETagMatchResult::Full: // Fact is favorite
				{
					if ( Options.bFilteringFavorites || Settings->bRemoveFavoritesFromMainTree == false )
					{
						bool bShouldFilterChildren = MatchText( SourceItem->Tag.ToString() ) == false;
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, bShouldFilterChildren );
					}
				}
				break;
			}
		}
	}

}


FFactTreeItem::~FFactTreeItem()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsDebuggerModule::Get().TryGetFactSubsystem() )
	{
		FactSubsystem->GetOnFactValueChangedDelegate( Tag ).Remove( Handle );
	}
}

void FFactTreeItem::InitPIE()
{
	Value.Reset();
	InitItem();
}

void FFactTreeItem::InitItem()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsDebuggerModule::Get().TryGetFactSubsystem() )
	{
		Handle = FactSubsystem->GetOnFactValueChangedDelegate( Tag ).AddRaw( this, &FFactTreeItem::HandleValueChanged );
		
		int32 FactValue;
		if ( FactSubsystem->TryGetFactValue( Tag, FactValue ) )
		{
			Value = FactValue;
		}
	}
}

void FFactTreeItem::HandleValueChanged( int32 NewValue )
{
	Value = NewValue;
	OnFactItemValueChanged.ExecuteIfBound( NewValue );
}

void FFactTreeItem::HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type )
{
	if ( Type == ETextCommit::Default || Type == ETextCommit::OnCleared )
	{
		return;
	}

	if ( UFactSubsystem* FactSubsystem = FSimpleFactsDebuggerModule::Get().TryGetFactSubsystem() )
	{
		FactSubsystem->ChangeFactValue( Tag, NewValue, EFactValueChangeType::Set );
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactsDebugger::Construct( const FArguments& InArgs )
{
	RootItem = MakeShared< FFactTreeItem >();
	FilteredRootItem = MakeShared< FFactTreeItem >();
	FavoritesRootItem = MakeShared< FFactTreeItem >();
		
	ChildSlot
	[
		SNew( SVerticalBox )

		// -------------------------------------------------------------------------------------------------------------
		// Presets menu

		+ SVerticalBox::Slot()
		.Padding( 2.f )
		.HAlign( HAlign_Right )
		.AutoHeight()
		[
			SAssignNew( ComboButton, SComboButton )
			.ToolTipText( LOCTEXT( "PresetsButton_Toolpit", "Open presets menu" ) )
			.OnGetMenuContent( this, &SFactsDebugger::HandleGeneratePresetsMenu )
			.ButtonContent()
			[
				SNew( SHorizontalBox )

				// -----------------------------------------------------------------------------------------------------
				// Preset icon
				+ SHorizontalBox::Slot()
				.Padding( 0, 1, 4, 0 )
				.AutoWidth()
				[
					SNew( SImage )
					.Image( FAppStyle::Get().GetBrush( "AssetEditor.SaveAsset" ) )
				]

				// -----------------------------------------------------------------------------------------------------
				// Preset text
				+ SHorizontalBox::Slot()
				.Padding( 0, 1, 0, 0 )
				.AutoWidth()
				[
					SNew( STextBlock )
					.Text( LOCTEXT( "PresetsButton", "Presets" ) )
				]
			]
		]
		
		// -------------------------------------------------------------------------------------------------------------
		// SearchBar
		
		+ SVerticalBox::Slot()
		.Padding( 2.f )
		.AutoHeight()
		[
			SNew( SHorizontalBox )

			// ---------------------------------------------------------------------------------------------------------
			// Search box
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Fill )
			.VAlign( VAlign_Center )
			.Padding(0.f, 1.f, 0.f, 1.f)
			[
				SAssignNew( SearchBox, SSearchBox )
				.HintText( LOCTEXT( "FactsDebugger_SearchHintText", "Search..." ) )
				.ToolTipText( LOCTEXT( "FactsDebugger_ToolTipText", "Search facts by tag. You can search by string ('Quest2.Trigger') or by several strings, separated by spaces ('Quest Trigger')\n"
													   "Press Enter to save this text as a toggle" ) )
				.OnTextChanged( this, &SFactsDebugger::HandleSearchTextChanged )
				.OnTextCommitted( this, &SFactsDebugger::HandleSearchTextCommitted )
			]

			// ---------------------------------------------------------------------------------------------------------
			// Options
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 4.f, 1.f, 0.f, 1.f )
			[
				SAssignNew( OptionsButton, SComboButton )
				.ContentPadding( 4.f )
				.ToolTipText( LOCTEXT( "ShowOptions_ToolTip", "Show options to affect the visibility of items in the Facts Debugger" ) )
				.ComboButtonStyle( FAppStyle::Get(), "SimpleComboButtonWithIcon" ) // Use the tool bar item style for this button
				.OnGetMenuContent( this, &SFactsDebugger::HandleGenerateOptionsMenu )
				.HasDownArrow( false )
				.ButtonContent()
				[
					SNew( SImage )
					.Image( FAppStyle::Get().GetBrush( "Icons.Settings") )
				]
			]
		]
		
		// -------------------------------------------------------------------------------------------------------------
		// Search toggles

		+ SVerticalBox::Slot()
		.Padding( 8.f, 4.f, 8.f, 4.f )
		.AutoHeight()
		[
			SAssignNew( SearchesHBox, SHorizontalBox )
			.Visibility_Lambda( [ this ]()
			{
				return SearchesContainer->GetChildren()->Num() > 0 ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
			} )

			// ---------------------------------------------------------------------------------------------------------
			// Search toggles

			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Fill )
			[
				SAssignNew( SearchesContainer, SWrapBox )
				.InnerSlotPadding( FVector2d{ 6, 4 } )
				.UseAllottedSize( true )
			]

			// ---------------------------------------------------------------------------------------------------------
			// Clear selected toggles button
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 8.f, 1.f, 2.f, 1.f )
			[
				SNew( SButton )
				.ToolTipText( LOCTEXT( "ClearSearchesButtonToolTip", "Clear all selected searches") )
				.Visibility_Lambda( [ this ]()
				{
					return IsAnySearchToggleActive() ? EVisibility::Visible : EVisibility::Collapsed;
				} )
				.OnClicked( this, &SFactsDebugger::HandleClearTogglesClicked )
				[
					SNew( STextBlock )
					.Text( LOCTEXT( "ClearSearchesButtonText", "Clear selected" ) )
				]
			]

			// -------------------------------------------------------------------------------------------------------------
			// Remove toggles button
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 8.f, 1.f, 2.f, 1.f )
			[
				SNew( SButton )
				.ToolTipText( LOCTEXT( "RemoveSearchesButtonToolTip", "Remove all searches from the facts Debugger") )
				.OnClicked_Lambda( [ this ]()
				{
					SearchesContainer->ClearChildren();
					CurrentSearchToggles.Empty();
					FilterItems();

					return FReply::Handled();
				} )
				[
					SNew( SImage )
					.Image( FAppStyle::Get().GetBrush( "Icons.X" ) )
				]
			]
		]
		

		// -------------------------------------------------------------------------------------------------------------
		// Facts trees

		+ SVerticalBox::Slot()
		.FillHeight( 1.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::Get().GetBrush( "Brushes.Panel" ) )
			[
				SAssignNew( Splitter, SSplitter )
				.Orientation( GetDefault< UFactsDebuggerSettingsLocal >()->Orientation )

				// -----------------------------------------------------------------------------------------------------
				// Favorites tree half
				
				+ SSplitter::Slot()
				[
					SNew( SVerticalBox )

					// -------------------------------------------------------------------------------------------------
					// Tree label
					
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateTreeLabel( LOCTEXT( "FavoritesTree_Label", "Favorites" ) )
					]
					
					// -------------------------------------------------------------------------------------------------
					// Tree panel
					
					+ SVerticalBox::Slot()
					.FillHeight( 1.f )
					[
						SNew ( SWidgetSwitcher )
						.WidgetIndex_Lambda( [ this ]()
						{
							return FavoritesRootItem->Children.Num() ? 0 : 1;
						} )

						// ---------------------------------------------------------------------------------------------
						// Favorites tree

						+ SWidgetSwitcher::Slot()
						.HAlign( HAlign_Fill )
						[
							CreateFactsTree( /*bIsFavoritesTree*/true )
						]

						// ---------------------------------------------------------------------------------------------
						// When no rows exist in view
						
						+ SWidgetSwitcher::Slot()
						.HAlign( HAlign_Fill )
						.Padding( 0.0f, 24.0f, 0.0f, 2.0f )
						[
							SNew( SRichTextBlock )
							.DecoratorStyleSet( &FFactsDebuggerStyle::Get() )
							.AutoWrapText( true )
							.Justification( ETextJustify::Center )
							.Text_Lambda( [ this ]()
							{
								if ( SFactsDebugger::FavoriteFacts.IsEmpty() )
								{
									return LOCTEXT( "EmptyFavoritesTree", "No Facts marked as \"Favorite\".\nClick <img src=\"RichText.StarOutline\"/> in \"All\" to add Fact to \"Favorites\"." );
								}

								return LOCTEXT( "EmptyFavoritesTree", "No matching Facts found. Check your filters." );
							} )
							+ SRichTextBlock::ImageDecorator()
						]
					]

					// -------------------------------------------------------------------------------------------------
					// Tree filter status
					
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateFilterStatusWidget( /*bIsFavoritesTree*/true )
					]
				]

				// -----------------------------------------------------------------------------------------------------
				// Main tree half
				
				+ SSplitter::Slot()
				[
					SNew( SVerticalBox )

					// -------------------------------------------------------------------------------------------------
					// Tree label
					
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateTreeLabel( LOCTEXT( "MainTree_Label", "All" ) )
					]

					// -------------------------------------------------------------------------------------------------
					// Tree panel
					
					+ SVerticalBox::Slot()
					.FillHeight( 1.f )
					[
						SNew ( SWidgetSwitcher )
						.WidgetIndex_Lambda( [ this ]()
						{
							return FilteredRootItem->Children.Num() ? 0 : 1;
						} )

						// ---------------------------------------------------------------------------------------------
						// Main tree
						
						+ SWidgetSwitcher::Slot()
						.HAlign( HAlign_Fill )
						[
							CreateFactsTree( /*bIsFavoritesTree*/false )
						]

						// ---------------------------------------------------------------------------------------------
						// When no rows exist in view
						
						+ SWidgetSwitcher::Slot()
						.HAlign( HAlign_Fill )
						.Padding( 0.0f, 24.0f, 0.0f, 2.0f )
						[
							SNew( STextBlock )
							.AutoWrapText( true )
							.Justification( ETextJustify::Center )
							.Text_Lambda( [ this ]()
							{
								if ( RootItem->Children.IsEmpty() )
								{
									return LOCTEXT( "EmptyFavoritesTree", "No Facts was found. Create Facts by adding subtags to \"Fact\" tag" );
								}

								return LOCTEXT( "EmptyFavoritesTree", "No matching Facts found. Check your filters." );
							} )
						]
					]

					// -------------------------------------------------------------------------------------------------
					// Tree filter status
					
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						CreateFilterStatusWidget( /*bIsFavoritesTree*/false )
					]
				]
			]
		]
	];

	TagChangedHandle = UGameplayTagsManager::OnEditorRefreshGameplayTagTree.AddSP( this, &SFactsDebugger::RebuildFactTreeItems );
	FSimpleFactsDebuggerModule::Get().OnGameInstanceStarted.BindRaw( this, &SFactsDebugger::HandleGameInstanceStarted );
	if ( InArgs._bIsGameStarted )
	{
		HandleGameInstanceStarted();
	}

	LoadFavorites();
	LoadSearchToggles();
	BuildFactTreeItems();
	FilterItems();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFactsDebugger::~SFactsDebugger()
{
	UGameplayTagsManager::OnEditorRefreshGameplayTagTree.Remove( TagChangedHandle );
	FSimpleFactsDebuggerModule::Get().OnGameInstanceStarted.Unbind();
}

void SFactsDebugger::LoadFactsPreset( UFactsPreset* InPreset )
{
	LoadedPreset = InPreset;
	LoadFactsPresetRecursive( InPreset, RootItem );
}

void SFactsDebugger::LoadFactsPresetRecursive( UFactsPreset* InPreset, const FFactTreeItemPtr& FactItem ) const
{
	if ( int32* PresetValue = InPreset->PresetValues.Find( FactItem->Tag ) )
	{
		FactItem->HandleNewValueCommited( *PresetValue, ETextCommit::Type::Default );
		FactItem->HandleValueChanged( *PresetValue );
	}

	for ( const FFactTreeItemPtr& ChildItem : FactItem->Children )
	{
		LoadFactsPresetRecursive( InPreset, ChildItem );
	}
}

int32 SFactsDebugger::CountAllFilteredItems( FFactTreeItemPtr ParentNode )
{
	int32 Result = 0;
	
	if ( GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree && SFactsDebugger::FavoriteFacts.Contains( ParentNode->Tag ) )
	{
		return Result;
	}

	for ( FFactTreeItemPtr Child : ParentNode->Children )
	{
		if ( int32 Temp = CountAllFilteredItems( Child ) )
		{
			Result += Temp;
		}
	}

	if ( GetDefault< UFactsDebuggerSettingsLocal >()->bCountOnlyLeafFacts && ParentNode->Children.Num() > 0 )
	{
		return Result;
	}

	if ( ParentNode->Tag.IsValid() )
	{
		Result++;
	}

	return Result;
}

int32 SFactsDebugger::CountAllFavoriteItems( FFactTreeItemPtr ParentNode, bool bIsParentFavorite )
{
	int32 Result = 0;
	
	if ( bIsParentFavorite == false && SFactsDebugger::FavoriteFacts.Contains( ParentNode->Tag ) )
	{
		bIsParentFavorite = true;
	}

	bool bHasFavoriteChild = false;
	for ( FFactTreeItemPtr Child : ParentNode->Children )
	{
		if ( int32 Temp = CountAllFavoriteItems( Child, bIsParentFavorite ) )
		{
			bHasFavoriteChild = true;
			Result += Temp;
		}
	}

	if ( GetDefault< UFactsDebuggerSettingsLocal >()->bCountOnlyLeafFacts && ParentNode->Children.Num() > 0 )
	{
		return Result;
	}
	
	if ( ParentNode->Tag.IsValid() && ( bIsParentFavorite || bHasFavoriteChild ) )
	{
		Result++;
	}

	return Result;
}

void SFactsDebugger::HandleGameInstanceStarted()
{
	InitItem( FilteredRootItem.ToSharedRef() );
	InitItem( FavoritesRootItem.ToSharedRef() );
}

void SFactsDebugger::InitItem( FFactTreeItemRef Item )
{
	Item->InitPIE();

	for ( FFactTreeItemPtr& ChildItem : Item->Children )
	{
		InitItem( ChildItem->AsShared() );
	}
}

TSharedRef< SWidget > SFactsDebugger::CreateTreeLabel( const FText& InLabel ) const
{
	return SNew( SBorder )
		.BorderImage( FAppStyle::Get().GetBrush( "Brushes.Background" ) )
		.Padding( 0.f, 0.f, 0.f, 3.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::Get().GetBrush( "Brushes.Header" ) )
			.Padding( 10.f, 4.f )
			[
				SNew( STextBlock )
				.Text( InLabel )
				.TextStyle( FAppStyle::Get(), "ButtonText" )
				.Font( FAppStyle::Get().GetFontStyle( "NormalFontBold" ) )
			]	
		];
}

TSharedRef< SWidget > SFactsDebugger::CreateFactsTree( bool bIsFavoritesTree )
{
	TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;
	FFactTreeItemPtr& ItemsSource = bIsFavoritesTree ? FavoritesRootItem : FilteredRootItem;
	
	return SAssignNew( TreeView, SFactsTreeView )
		.TreeItemsSource( &ItemsSource->Children )
		.OnGenerateRow( this, &SFactsDebugger::OnGenerateWidgetForFactsTreeView )
		.OnGetChildren( this, &SFactsDebugger::OnGetChildren)
		.OnExpansionChanged( this, &SFactsDebugger::HandleExpansionChanged, false, bIsFavoritesTree )
		.OnSetExpansionRecursive( this, &SFactsDebugger::HandleExpansionChanged, true, bIsFavoritesTree )
		.OnGeneratePinnedRow( this, &SFactsDebugger::HandleGeneratePinnedTreeRow )
		.ShouldStackHierarchyHeaders_Lambda( []() { return GetDefault< UFactsDebuggerSettingsLocal >()->bShouldStackHierarchyHeaders; } )
		.OnContextMenuOpening( this, &SFactsDebugger::HandleGenerateMainContextMenu )
		.SelectionMode( ESelectionMode::Type::Single )
		.HeaderRow
		(
			CreateHeaderRow( bIsFavoritesTree )
		);
}

TSharedRef< SHeaderRow > SFactsDebugger::CreateHeaderRow( bool bIsFavoritesTree ) const
{
	return SNew( SHeaderRow )

		+ SHeaderRow::Column( "Favorites" )
		.FixedWidth( 24.f )
		.HAlignHeader( HAlign_Center )
		.VAlignHeader( VAlign_Center )
		.HAlignCell( HAlign_Center )
		.VAlignCell( VAlign_Center )
		[
			SNew( SImage )
			.Image_Lambda( [ bIsFavoritesTree ]()
			{
				return bIsFavoritesTree
					? FAppStyle::Get().GetBrush( "Icons.Star" )
					: GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree
						? FFactsDebuggerStyle::Get().GetBrush( "Icons.Star.Outline" )
						: FFactsDebuggerStyle::Get().GetBrush( "Icons.Star.OutlineFilled" );
			} )
		]
		
		+ SHeaderRow::Column( "Tag" )
		.SortPriority( EColumnSortPriority::Secondary )
		.DefaultLabel( LOCTEXT( "TagColumn", "Tag" ) )

		+ SHeaderRow::Column( "Value" )
		.ManualWidth( 90.f )
		.DefaultLabel( LOCTEXT( "ValueColumn", "Value" ) )
		.DefaultTooltip( LOCTEXT( "ValueColumn_ToolTip", "Current value of this fact. Undefined means that value for this fact was not yet set" ) );
}

TSharedRef< SWidget > SFactsDebugger::CreateFilterStatusWidget( bool bIsFavoritesTree ) const
{
	return SNew( SBorder )
		.BorderImage( FAppStyle::Get().GetBrush( "Brushes.Header" ) )
		.VAlign( VAlign_Center )
		.HAlign( HAlign_Left )
		.Padding( 10.f, 4.f )
		[
			SNew( STextBlock )
			.Text( this, &SFactsDebugger::GetFilterStatusText, bIsFavoritesTree )
			.ColorAndOpacity( this, &SFactsDebugger::GetFilterStatusTextColor, bIsFavoritesTree )
		];
}

TSharedRef<ITableRow> SFactsDebugger::OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr InItem, const TSharedRef<STableViewBase>& TableViewBase )
{
	class SFactTreeItem : public SMultiColumnTableRow< FFactTreeItemPtr >
	{
	public:
		SLATE_BEGIN_ARGS( SFactTreeItem ) {}
		SLATE_END_ARGS()

		SFactTreeItem()
			: FavoriteBrush( FAppStyle::Get().GetBrush( "Icons.Star" ) )
			, NormalBrush( FFactsDebuggerStyle::Get().GetBrush( "Icons.Star.Outline" ) )
			, EvenColor( USlateThemeManager::Get().GetColor( EStyleColor::Recessed ) )
			, OddColor( USlateThemeManager::Get().GetColor( EStyleColor::Background ) )
			, Animation( FCurveSequence(0.f, AnimationDuration, ECurveEaseFunction::Linear ) )
			, AnimationColor( FFactsDebuggerStyle::Get().GetColor( "Colors.FactChanged" ) )
		{ }

		void Construct( const FArguments& InArgs, const TSharedRef< STableViewBase > InOwnerTable, TSharedPtr< SFactsDebugger > InFactsDebugger, FFactTreeItemPtr InItem )
		{
			Item = InItem;
			bShowFullName = GetDefault< UFactsDebuggerSettingsLocal >()->bShowFullFactNames;
			FactsDebugger = InFactsDebugger;

			SMultiColumnTableRow::Construct( FSuperRowType::FArguments()
				.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>( "ContentBrowser.AssetListView.ColumnListTableRow" ) ), InOwnerTable );

			Item->OnFactItemValueChanged.BindRaw( this, &SFactTreeItem::HandleItemValueChanged );
		}

		virtual void ResetRow() override
		{
			Item->OnFactItemValueChanged.Unbind();
		}
		
		virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& InColumnName ) override
		{
			if ( InColumnName == "Favorites" )
			{
				return SNew( SButton )
					.ButtonStyle( &FAppStyle::Get(), "NoBorder" )
					.OnClicked( this, &SFactTreeItem::HandleFavoriteClicked )
					[
						SNew( SImage )
						.ColorAndOpacity( this, &SFactTreeItem::GetItemColor )
						.Image( this, &SFactTreeItem::GetItemBrush )
					];
			}
			else if ( InColumnName == "Tag" )
			{
				return SNew( SHorizontalBox )
					
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SFactsExpanderArrow, SharedThis( this ) )
					]
					
					+SHorizontalBox::Slot()
					.FillWidth( 1.f )
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.ColorAndOpacity( Item->Tag.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground() )
						.Text( FText::FromString( bShowFullName ? Item->Tag.ToString() : Item->TagNode->GetSimpleTagName().ToString() ) )
						.HighlightText_Lambda( [ this ](){ return FactsDebugger->CurrentSearchText; } )
					];
			}
			else if ( InColumnName == "Value" )
			{
				return SNew( SBox )
					.Padding( 1.f )
					[
						SNew( SNumericEntryBox< int32 > )
						.Value_Raw( Item.Get(), &FFactTreeItem::GetValue )
						.OnValueCommitted( FOnInt32ValueCommitted::CreateRaw( Item.Get(), &FFactTreeItem::HandleNewValueCommited ) )
						.UndeterminedString( LOCTEXT( "FactUndefinedValue", "undefined" ) )
					];
			}
			else
			{
				return SNew( STextBlock ).Text( LOCTEXT( "UnknownColumn", "Unknown Column" ) );
			}
		}
		
		FReply HandleFavoriteClicked()
		{
			if ( SFactsDebugger::FavoriteFacts.Contains( Item->Tag ) )
			{
				SFactsDebugger::FavoriteFacts.Remove( Item->Tag );
			}
			else
			{
				SFactsDebugger::FavoriteFacts.Add( Item->Tag );
			}
			
			Utils::AllFilteredFactsCount = FactsDebugger->CountAllFilteredItems( FactsDebugger->RootItem );
			Utils::AllFavoriteFactsCount = FactsDebugger->CountAllFavoriteItems( FactsDebugger->RootItem, false );

			FactsDebugger->SaveFavorites();
			FactsDebugger->FilterItems();
			return FReply::Handled();
		}

		void HandleItemValueChanged( int32 NewValue )
		{
			Animation.Play( AsShared() );
		}

		const virtual FSlateBrush* GetBorder() const override
		{
			if ( Animation.IsPlaying() )
			{
				const bool bEvenEntryIndex = ( IndexInList % 2 == 0 );
				
				const float Progress = FMath::Min( Animation.GetLerp(), 1 - Animation.GetLerp() );
				ChangedBrush.TintColor = FMath::Lerp( bEvenEntryIndex ? EvenColor : OddColor, AnimationColor, Progress );
				return &ChangedBrush;
			}

			return STableRow< FFactTreeItemPtr >::GetBorder();
		}

		const FSlateBrush* GetItemBrush() const
		{
			return IsFavorite() ? FavoriteBrush : NormalBrush;
		}

		FSlateColor GetItemColor() const
		{
			const bool bIsSelected = FactsDebugger->FactsTreeView->IsItemSelected( Item.ToSharedRef() );
		
			if ( IsFavorite() == false )
			{
				if ( IsHovered() == false && bIsSelected == false )
				{
					return FLinearColor::Transparent;
				}
			}
		
			return IsHovered() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
		}

		bool IsFavorite() const
		{
			if ( Item.IsValid() == false )
			{
				return false;
			}

			auto MatchFavorites = [ Tag = Item->Tag ]( FFactTag FavoriteTag )
			{
				return Tag == FavoriteTag;
			};
			
			return Algo::AnyOf( SFactsDebugger::FavoriteFacts, MatchFavorites );
		}

	private:

		FFactTreeItemPtr Item;
		bool bShowFullName = false;
		TSharedPtr< SFactsDebugger > FactsDebugger;

		const FSlateBrush* FavoriteBrush = nullptr;
		const FSlateBrush* NormalBrush = nullptr;
		mutable FSlateColorBrush ChangedBrush = FSlateColorBrush( FStyleColors::Background );
		
		FLinearColor EvenColor;
		FLinearColor OddColor;

		const float AnimationDuration = 1.f;
		FCurveSequence Animation;
		FLinearColor AnimationColor;
	};

	if ( InItem.IsValid() )
	{
		return SNew( SFactTreeItem, TableViewBase, SharedThis( this ), InItem );
	}
	else
	{
		return SNew( STableRow< TSharedPtr< FString > >, TableViewBase )
			[
				SNew( STextBlock )
				.Text( LOCTEXT( "UnknownItemType", "Unknown Item Type" ) )
			];		
	}
}

TSharedRef< ITableRow > SFactsDebugger::HandleGeneratePinnedTreeRow( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase )
{
	bool bShowFullName = GetDefault< UFactsDebuggerSettingsLocal >()->bShowFullFactNames;
	
	return SNew( STableRow< TSharedPtr< FString > >, TableViewBase )
		[
			SNew( SBox )
			.HeightOverride( 22.f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromString( bShowFullName ? FactTreeItem->Tag.ToString() : FactTreeItem->TagNode->GetSimpleTagName().ToString() ) )
			]
		];
}

void SFactsDebugger::OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray<FFactTreeItemPtr>& Children )
{
	if ( FactTreeItem.IsValid() )
	{
		Children.Append( FactTreeItem->Children );
	}
}

void SFactsDebugger::HandleExpansionChanged(FFactTreeItemPtr FactTreeItem, bool bInExpanded, bool bRecursive, bool bIsFavoritesTree)
{
	TSet< FFactTag >& ExpandedFacts = bIsFavoritesTree ? FavoritesExpandedFacts : MainExpandedFacts;
	
	if ( bPersistExpansionChange && FactTreeItem->Children.Num() )
    {
    	if ( bInExpanded )
    	{
    		ExpandedFacts.Add( FactTreeItem->Tag );
    	}
    	else
    	{
    		ExpandedFacts.Remove( FactTreeItem->Tag );
    	}

    	if ( bRecursive )
    	{
    		// if it is not recursive, then it is already expanded
    		TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;
    		TreeView->SetItemExpansion( FactTreeItem, bInExpanded );
    		
    		for ( FFactTreeItemPtr Child : FactTreeItem->Children )
    		{
    			HandleExpansionChanged( Child, bInExpanded, bRecursive, bIsFavoritesTree );
    		}
    	}
    }
}

FText SFactsDebugger::GetFilterStatusText( bool bIsFavoritesTree ) const
{
	int32 AllFactsCount = bIsFavoritesTree ? Utils::AllFavoriteFactsCount : Utils::AllFilteredFactsCount;
	int32 CurrentFactCount = bIsFavoritesTree ? Utils::CurrentFavoriteFactsCount : Utils::CurrentFilteredFactsCount;
	const TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;

	if ( CurrentSearchText.IsEmpty() && IsAnySearchToggleActive() == false )
	{
		return FText::Format( LOCTEXT( "ShowingAllFacts", "{0} facts" ), FText::AsNumber( AllFactsCount ) );
	}

	if ( TreeView->GetRootItems().IsEmpty() )
	{
		return FText::Format( LOCTEXT( "NoMatchingFacts", "No matching facts ({0} total)" ), FText::AsNumber( AllFactsCount ) );
	}
	
	return FText::Format( LOCTEXT( "ShowingFilteredFacts", "{0} facts ({1} total)" ), FText::AsNumber( CurrentFactCount ), FText::AsNumber( AllFactsCount ) );

}

FSlateColor SFactsDebugger::GetFilterStatusTextColor( bool bIsFavoritesTree ) const
{
	const TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;

	if ( CurrentSearchText.IsEmpty() && IsAnySearchToggleActive() == false )
	{
		return FSlateColor::UseForeground();
	}

	if ( TreeView->GetRootItems().IsEmpty() )
	{
		return FStyleColors::AccentRed;
	}

	return FStyleColors::AccentGreen;
}

TSharedRef<SWidget> SFactsDebugger::HandleGeneratePresetsMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };

	FUIAction PresetNameAction = FUIAction();
	PresetNameAction.CanExecuteAction = FCanExecuteAction::CreateLambda( []() { return false; } );

	MenuBuilder.AddMenuEntry(
		LOCTEXT( "CurrentPreset", "Current preset:"),
		LOCTEXT( "CurrentPreset_ToolTip", "Current"),
		FSlateIcon(),
		PresetNameAction,
		NAME_None,
		EUserInterfaceActionType::None
	);

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT( "SavePreset", "Save preset" ),
		LOCTEXT( "SavePreset_ToolTip", "Save the current preset" ),
		FSlateIcon(FAppStyle::Get().GetStyleSetName(), "AssetEditor.SaveAsset" ),
		FUIAction(FExecuteAction::CreateLambda( [ this ]() { } ) )
	);

	MenuBuilder.BeginSection( NAME_None, LOCTEXT( "LoadPreset_MenuSection", "Load preset" ));
	{
		TArray< FAssetData > AssetData;
		IAssetRegistry::Get()->GetAssetsByClass( UFactsPreset::StaticClass()->GetClassPathName(), AssetData );

		TSharedPtr< SFactsPresetPicker > PresetPicker;
		TSharedRef< SWidget > MenuWidget = SNew( SBox )
			.WidthOverride( 300.f )
			.HeightOverride( 300.f )
			.Padding( 2.f )
			[
				SAssignNew( PresetPicker, SFactsPresetPicker, AssetData )
				.OnPresetSelected_Lambda( [ this ]( UFactsPreset* Preset )
				{
					LoadFactsPreset( Preset );
					check( ComboButton );
					ComboButton->SetIsOpen( false );
				})
			];

		ComboButton->SetMenuContentWidgetToFocus( PresetPicker->GetWidgetToFocusOnOpen() );

		MenuBuilder.AddWidget( MenuWidget, FText(), true, false );
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget(  );
}

TSharedRef< SWidget > SFactsDebugger::HandleGenerateOptionsMenu()
{
	FMenuBuilder MenuBuilder( false, nullptr );

	MenuBuilder.BeginSection( "", LOCTEXT( "Options_HierarchySectionName", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ExpandAll", "Expand All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::HandleExpandAllClicked ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_CollapseAll", "Collapse All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::HandleCollapseAllClicked ) )
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowHierarchy", "Stack Hierarchy Headers" ),
			LOCTEXT( "Options_ShowHierarchy_ToolTip", "Toggle pinning of the hierarchy of items at the top of the outliner" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShouldStackHierarchyHeaders ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bShouldStackHierarchyHeaders; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);
	}

	MenuBuilder.EndSection();
	
	MenuBuilder.BeginSection( "", LOCTEXT( "Options_ShowSectionName", "Show" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowRootTag", "Show Root Fact Tag" ),
			LOCTEXT( "Options_ShowRootTag_ToolTip", "Show Root Fact Tag" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowRootFactTag ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bShowRootFactTag; })
				),
			NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowFullNames", "Show Full Fact Names" ),
			LOCTEXT( "Options_ShowFullNames_ToolTip", "Show Full Fact Names" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowFullFactNames ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bShowFullFactNames; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_RemoveFavorites", "Remove Favorites from Main Tree" ),
			LOCTEXT( "Options_RemoveFavorites_ToolTip", "Remove Favorites from Main Tree" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bRemoveFavoritesFromMainTree ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_CountLeafs", "Count Only Leaf Facts" ),
			LOCTEXT( "Options_CountLeafs_ToolTip", "Count only leaf Facts for numbers displayed below the trees" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bCountOnlyLeafFacts ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bCountOnlyLeafFacts; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "", LOCTEXT( "Options_OrientationSectionHeader", "Orientation" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_Horizontal", "Horizontal" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateRaw( this, &SFactsDebugger::HandleOrientationChanged, Orient_Horizontal ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->Orientation == Orient_Horizontal; })
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_Horizontal", "Vertical" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateRaw( this, &SFactsDebugger::HandleOrientationChanged, Orient_Vertical ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->Orientation == Orient_Vertical; })
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

TSharedPtr< SWidget > SFactsDebugger::HandleGenerateMainContextMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };

	MenuBuilder.BeginSection( "", LOCTEXT( "ContextMenu_TreeSection", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_ExpandTree", "Expand Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::SetItemsExpansion, FactsTreeView, FilteredRootItem->Children, true, true ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_CollapseTree", "Collapse Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::SetItemsExpansion, FactsTreeView, FilteredRootItem->Children, false, true ) )
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SFactsDebugger::HandleGenerateFavoritesContextMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };

	MenuBuilder.BeginSection( "", LOCTEXT( "ContextMenu_TreeSection", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_ExpandTree", "Expand Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::SetItemsExpansion, FavoriteFactsTreeView, FavoritesRootItem->Children, true, true ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_CollapseTree", "Collapse Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsDebugger::SetItemsExpansion, FavoriteFactsTreeView, FavoritesRootItem->Children, false, true ) )
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection( "", LOCTEXT( "ContextMenu_FavoritesSection", "Favorites" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_RemoveAllFavorites", "Remove All Favorites" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( [ this ]()
				{
					ClearFavoritesRecursive( FavoritesRootItem );

					Utils::AllFilteredFactsCount = 0;
					Utils::AllFavoriteFactsCount = CountAllFavoriteItems( RootItem, false );

					SaveFavorites();
					FilterItems();
				} ),
				FCanExecuteAction::CreateLambda( [ this ]()
				{
					return Utils::AllFavoriteFactsCount > 0;
				} )
			)
		);
		
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_RemoveItemFavorites", "Remove All Favorites for Item" ),
			LOCTEXT( "ContextMenu_RemoveItemFavorites_ToolTip", "Remove all children facts that are Favorites (including this item)" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( [ this ]()
				{
					TArray< FFactTreeItemPtr > SelectedItems = FavoriteFactsTreeView->GetSelectedItems();
					checkf( SelectedItems.Num() <= 1, TEXT( "SelectionMode for Favorites tree should be ESelectionMode::Type::Single" ) );

					if ( SelectedItems.IsEmpty() )
					{
						return;
					}
					
					ClearFavoritesRecursive( SelectedItems[0] );

					Utils::AllFilteredFactsCount = CountAllFilteredItems( RootItem );
					Utils::AllFavoriteFactsCount = CountAllFavoriteItems( RootItem, false );

					SaveFavorites();
					FilterItems();
				} ),
				FCanExecuteAction::CreateLambda( [ this ]()
				{
					TArray< FFactTreeItemPtr > SelectedItems = FavoriteFactsTreeView->GetSelectedItems();
					checkf( SelectedItems.Num() <= 1, TEXT( "SelectionMode for Favorites tree should be ESelectionMode::Type::Single" ) );

					if ( SelectedItems.IsEmpty() )
					{
						return false;
					}

					return HasFavoritesRecursive( SelectedItems[0] );
				} )
			)
		);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SFactsDebugger::ClearFavoritesRecursive( FFactTreeItemPtr Item )
{
	if ( SFactsDebugger::FavoriteFacts.Contains( Item->Tag ) )
	{
		SFactsDebugger::FavoriteFacts.Remove( Item->Tag );
	}

	for ( FFactTreeItemPtr Child : Item->Children )
	{
		ClearFavoritesRecursive( Child );
	}
}

bool SFactsDebugger::HasFavoritesRecursive( FFactTreeItemPtr Item )
{
	if ( SFactsDebugger::FavoriteFacts.Contains( Item->Tag ) )
	{
		return true;
	}

	for ( FFactTreeItemPtr Child : Item->Children )
	{
		if ( HasFavoritesRecursive( Child ) )
		{
			return true;
		}
	}

	return false;
}

void SFactsDebugger::HandleSearchTextChanged( const FText& SearchText )
{
	CurrentSearchText = SearchText;
	FilterItems();
}

void SFactsDebugger::HandleSearchTextCommitted( const FText& SearchText, ETextCommit::Type Type )
{
	if ( Type != ETextCommit::Type::OnEnter )
	{
		return;
	}
	
	if ( SearchText.IsEmpty() )
	{
		return;
	}

	TArray< FString > ExistingStrings;
	ExistingStrings.Reserve( CurrentSearchToggles.Num() );

	for ( const SFactsSearchToggleRef& SearchToggle : CurrentSearchToggles )
	{
		const FText ToggleText = SearchToggle->GetSearchText();
		ExistingStrings.Add( ToggleText.ToString() );

		if ( SearchText.EqualToCaseIgnored( ToggleText ) )
		{
			SearchToggle->SetIsButtonChecked( true );
		}
	}

	if ( ExistingStrings.Contains( SearchText.ToString() ))
	{
		return;
	}

	SFactsSearchToggleRef NewSearchToggle = ConstructSearchToggle( SearchText );
	CurrentSearchToggles.Add( NewSearchToggle );

	RefreshSearchToggles();
	SearchBox->SetText( FText::GetEmpty() );

	SaveSearchToggles();
}

void SFactsDebugger::FilterItems()
{
	FilteredRootItem.Reset();
	FavoritesRootItem.Reset();

	// Parse filter strings
	TArray< FString > ActiveTogglesText;
	for ( const SFactsSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		if ( SearchToggle->GetIsToggleChecked() )
		{
			ActiveTogglesText.Add( SearchToggle->GetSearchText().ToString() );
		}
	}

	TArray< FString > Tokens;
	CurrentSearchText.ToString().ParseIntoArray( Tokens, TEXT(  " "  ) );

	// Reset containers
	FilteredRootItem = MakeShared< FFactTreeItem >();
	FavoritesRootItem = MakeShared< FFactTreeItem >();

	// Filtering
	Utils::FFilterOptions Options{ ActiveTogglesText, Tokens, false };
	Utils::FilterFactItemChildren( RootItem->Children,  FilteredRootItem->Children, Options );

	Utils::FFilterOptions FavoritesOptions{ ActiveTogglesText, Tokens, true };
	Utils::FilterFactItemChildren( RootItem->Children,  FavoritesRootItem->Children, FavoritesOptions );

	Utils::CurrentFilteredFactsCount = CountAllFilteredItems( FilteredRootItem );
	Utils::CurrentFavoriteFactsCount = CountAllFavoriteItems( FavoritesRootItem, false );
	
	FactsTreeView->SetTreeItemsSource( &FilteredRootItem->Children );
	FavoriteFactsTreeView->SetTreeItemsSource( &FavoritesRootItem->Children );

	if ( ActiveTogglesText.Num() || Tokens.Num() )
	{
		SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true, false );
		SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true, false );
	}
	else
	{
		// RestoreExpansionState();
		SetDefaultItemsExpansion( FactsTreeView, FilteredRootItem->Children, SFactsDebugger::MainExpandedFacts );
		SetDefaultItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, SFactsDebugger::FavoritesExpandedFacts );
	}

	FactsTreeView->RequestTreeRefresh();
	FavoriteFactsTreeView->RequestTreeRefresh();
}

void SFactsDebugger::HandleExpandAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true, true );
	OptionsButton->SetIsOpen( false );
}

void SFactsDebugger::HandleCollapseAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, false, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, false, true );
	OptionsButton->SetIsOpen( false );
}

void SFactsDebugger::SetItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, TArray<FFactTreeItemPtr> FactItems, bool bShouldExpand, bool bPersistExpansion )
{
	TGuardValue< bool > PersistExpansionChangeGuard( bPersistExpansionChange, bPersistExpansion );

	for ( const FFactTreeItemPtr& Item : FactItems )
	{
		TreeView->SetItemExpansion( Item, bShouldExpand );
		SetItemsExpansion( TreeView, Item->Children, bShouldExpand, bPersistExpansion );
	}
}

void SFactsDebugger::RestoreExpansionState()
{
	TGuardValue< bool > PersistExpansionChangeGuard( bPersistExpansionChange, false );
	
	const UFactsDebuggerSettingsLocal* Settings = GetDefault< UFactsDebuggerSettingsLocal >();

	TArray< FFactTreeItemPtr > Path;
	for ( const FFactTag& FactTag : MainExpandedFacts )
	{
		Path.Reset();
		if ( FindItemByTagRecursive( FilteredRootItem, FactTag, Path) )
		{
			FactsTreeView->SetItemExpansion( Path.Last(), true );
		}
	}

	for ( const FFactTag& FactTag : FavoritesExpandedFacts )
	{
		Path.Reset();
		if ( FindItemByTagRecursive( FavoritesRootItem, FactTag, Path ) )
		{
			FavoriteFactsTreeView->SetItemExpansion( Path.Last(), true );
		}
	}
}

void SFactsDebugger::SetDefaultItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, const TArray< FFactTreeItemPtr >& FactItems, const TSet< FFactTag >& ExpandedFacts )
{
	TGuardValue< bool > PersistExpansionChangeGuard( bPersistExpansionChange, false );

	for ( const FFactTreeItemPtr& Item : FactItems )
	{
		if ( ExpandedFacts.Contains( Item->Tag ) )
		{
			TreeView->SetItemExpansion( Item, true );
		}
		SetDefaultItemsExpansion( TreeView, Item->Children, ExpandedFacts );
	}
}

bool SFactsDebugger::FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag,
	TArray<FFactTreeItemPtr>& OutPath )
{
	OutPath.Push( Item );

	if ( Item->Tag == Tag )
	{
		return true;
	}

	for ( const FFactTreeItemPtr& ChildItem : Item->Children )
	{
		if ( FindItemByTagRecursive( ChildItem, Tag, OutPath ) )
		{
			return true;
		}
	}

	OutPath.Pop();

	return false;
}

void SFactsDebugger::CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates )
{
	for ( FSearchToggleState& ToggleState : SearchToggleStates )
	{
		SFactsSearchToggleRef NewSearchToggle = ConstructSearchToggle( ToggleState.SearchText, ToggleState.bIsToggleChecked );
		CurrentSearchToggles.Add( NewSearchToggle );
	}

	RefreshSearchToggles();
}

SFactsSearchToggleRef SFactsDebugger::ConstructSearchToggle( const FText& InSearchText, bool bInChecked )
{
	return SNew( SFactsSearchToggle, InSearchText )
			.OnClickedOnce( this, &SFactsDebugger::HandleSearchToggleClicked )
			.OnRightButtonClicked( this, &SFactsDebugger::HandleRemoveSearchToggle )
			.OnAltClicked( this, &SFactsDebugger::HandleRemoveSearchToggle )
			.IsToggleChecked( bInChecked );
}

FReply SFactsDebugger::HandleRemoveSearchToggle()
{
	CleanupSearchesMarkedForDelete();
	RefreshSearchToggles();
	FilterItems();
	SaveSearchToggles();

	return FReply::Handled();
}

void SFactsDebugger::CleanupSearchesMarkedForDelete()
{
	CurrentSearchToggles.RemoveAllSwap( []( const SFactsSearchToggleRef& SearchToggle )
	{
		return SearchToggle->GetIsMarkedForDelete();
	} );
}

void SFactsDebugger::RefreshSearchToggles()
{
	SearchesContainer->ClearChildren();
	
	for ( const SFactsSearchToggleRef& SearchToggle: CurrentSearchToggles )
	{
		SearchesContainer->AddSlot()
		[
			SearchToggle
		];
	}
}

FReply SFactsDebugger::HandleClearTogglesClicked()
{
	for ( const SFactsSearchToggleRef& SearchToggle : CurrentSearchToggles )
	{
		SearchToggle->SetIsButtonChecked( false );
	}

	FilterItems();
	SaveSearchToggles();

	return FReply::Handled();
}

FReply SFactsDebugger::HandleSearchToggleClicked()
{
	FilterItems();
	SaveSearchToggles();

	return FReply::Handled();
}

TArray<FSearchToggleState> SFactsDebugger::GetSearchToggleStates()
{
	TArray< FSearchToggleState > SearchToggleStates;
	for ( const SFactsSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		SearchToggleStates.Emplace( SearchToggle->GetIsToggleChecked(), SearchToggle->GetSearchText() );
	}

	return SearchToggleStates;
}

bool SFactsDebugger::IsAnySearchToggleActive() const
{
	for ( const SFactsSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		if ( SearchToggle->GetIsToggleChecked() )
		{
			return true;
		}
	}

	return false;
}

void SFactsDebugger::BuildFactTreeItems()
{
	RootItem = MakeShared< FFactTreeItem >();

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	TSharedPtr< FGameplayTagNode > Node = Manager.FindTagNode( FFactTag::GetRootTag() );

	if ( GetDefault< UFactsDebuggerSettingsLocal >()->bShowRootFactTag )
	{
		BuildFactItem( RootItem, Node );
	}
	else
	{
		for ( TSharedPtr< FGameplayTagNode >& ChildNode : Node->GetChildTagNodes())
		{
			BuildFactItem( RootItem, ChildNode );
		}
	}

	Utils::AllFilteredFactsCount = CountAllFilteredItems( RootItem );
	Utils::AllFavoriteFactsCount = CountAllFavoriteItems( RootItem, false );

	FilteredRootItem = RootItem;
	FavoritesRootItem = MakeShared< FFactTreeItem >();
}

FFactTreeItemPtr SFactsDebugger::BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode )
{
	FFactTreeItemPtr NewItem = MakeShared< FFactTreeItem >(  );
	NewItem->Tag = FFactTag::ConvertChecked( ThisNode->GetCompleteTag() );
	NewItem->TagNode = ThisNode;
	NewItem->Children.Reserve( ThisNode->GetChildTagNodes().Num() );
	ParentNode->Children.Add( NewItem );
	
	for ( TSharedPtr< FGameplayTagNode > Node : ThisNode->GetChildTagNodes() )
	{
		BuildFactItem( NewItem, Node );
	}

	return NewItem;
}

void SFactsDebugger::RebuildFactTreeItems()
{
	BuildFactTreeItems();
	FilterItems();
}

void SFactsDebugger::LoadSearchToggles()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	CreateDefaultSearchToggles( Settings->ToggleStates );
}

void SFactsDebugger::SaveSearchToggles()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->ToggleStates = GetSearchToggleStates();
	Settings->SaveConfig();
}

void SFactsDebugger::LoadFavorites()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	SFactsDebugger::FavoriteFacts = Settings->FavoriteFacts;
}

void SFactsDebugger::SaveFavorites()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->FavoriteFacts = SFactsDebugger::FavoriteFacts;
	Settings->SaveConfig();
}

void SFactsDebugger::HandleOrientationChanged( EOrientation Orientation )
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->Orientation = Orientation;
	Settings->SaveConfig();
	
	Splitter->SetOrientation( Orientation );
}

#undef LOCTEXT_NAMESPACE