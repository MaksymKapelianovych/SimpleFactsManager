// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactDebugger.h"

#include "FactDebuggerSettingsLocal.h"
#include "FactDebuggerStyle.h"
#include "Styling/StyleColors.h"

#include "FactPreset.h"
#include "FactSubsystem.h"
#include "GameplayTagsManager.h"
#include "SFactSearchBox.h"
#include "SFactSearchToggle.h"
#include "SFactExpanderArrow.h"
#include "SFactPresetPicker.h"
#include "SimpleFactsDebugger.h"
#include "SlateOptMacros.h"
#include "Algo/AllOf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/SRichTextBlock.h"

#define LOCTEXT_NAMESPACE "FactDebugger"

TSet< FFactTag > SFactDebugger::MainExpandedFacts;
TSet< FFactTag > SFactDebugger::FavoritesExpandedFacts;
TArray< FFactTag > SFactDebugger::FavoriteFacts;

// local duplicates for UFactDebuggerSettingsLocal
namespace Settings
{
	bool bShowRootFactTag = false;
	bool bShowFullFactNames = false;
	bool bShouldStackHierarchyHeaders = false;
	bool bShowFavoritesInMainTree = false;
	bool bShowOnlyLeafFacts = false;
	bool bShowOnlyDefinedFacts = false;	
}

#define TOGGLE_FACT_SETTING( PropertyName ) \
	[ this ]() { \
		Settings::PropertyName = !Settings::PropertyName; \
		SaveSettings(); \
		 \
		RebuildFactTreeItems(); \
	}


namespace Utils
{
	int32 AllFilteredFactsCount = 0;
	int32 AllFavoriteFactsCount = 0;
	
	int32 CurrentFilteredFactsCount = 0;
	int32 CurrentFavoriteFactsCount = 0;
	
	
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

	enum class ETagMatchType
	{
		None,
		Parent,
		Child,
		Full
	};
	ETagMatchType MatchFavorites( FFactTag CheckedTag )
	{
		bool bParentMatch = false;
		bool bChildMatch = false;
			
		for ( FFactTag FavoriteFact : SFactDebugger::FavoriteFacts )
		{
			if ( CheckedTag == FavoriteFact ) {	return ETagMatchType::Full; }

			if ( CheckedTag.MatchesTag( FavoriteFact ) ) { bParentMatch = true; }
			else if ( FavoriteFact.MatchesTag( CheckedTag ) ) { bChildMatch = true; }
		}

		if ( bParentMatch ) { return ETagMatchType::Parent; }
		else if ( bChildMatch ) { return ETagMatchType::Child; }

		return ETagMatchType::None;
	};

	struct FFilterOptions
	{
		TArray< FString > SearchToggleStrings;
		TArray< FString > SearchBarStrings;
		bool bIsPlaying;
	};

	void FilterFavoritesFactItemChildren( TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );
	void FilterMainFactItemChildren( TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );

	void CopyItem( FFactTreeItemPtr SourceItem, TArray< FFactTreeItemPtr >& OutDestArray )
	{
		FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
		*NewItem = *SourceItem;
		NewItem->InitItem();
	}

	void CopyItemIfMainChildrenMatch( FFactTreeItemPtr SourceItem, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		TArray< FFactTreeItemPtr > FilteredChildren;
		FilterMainFactItemChildren( SourceItem->Children, FilteredChildren, Options );
		if ( FilteredChildren.Num() )
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
			NewItem->InitItem();
			NewItem->Children = FilteredChildren;
		}
	}

	void CopyItemIfFavoritesChildrenMatch( FFactTreeItemPtr SourceItem, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		TArray< FFactTreeItemPtr > FilteredChildren;
		FilterFavoritesFactItemChildren( SourceItem->Children, FilteredChildren, Options );
		if ( FilteredChildren.Num() )
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
			NewItem->InitItem();
			NewItem->Children = FilteredChildren;
		}
	}
	
	void FilterFavoritesFactItemChildren( TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		auto MatchText = [ &Options ]( const FString& TagString )
		{
			return MatchSearchBox( Options.SearchBarStrings, TagString ) && MatchSearchToggle( Options.SearchToggleStrings, TagString );
		};

		for ( const FFactTreeItemPtr& SourceItem : SourceArray )
		{
			bool bNeedsFilteringByText = false;

			if ( Settings::bShowOnlyDefinedFacts && Options.bIsPlaying )
			{
				if ( SourceItem->Children.Num() > 0 )
				{
					CopyItemIfFavoritesChildrenMatch( SourceItem, OutDestArray, Options );
					continue;
				}
				else if ( SourceItem->Value.IsSet() == false )
				{
					continue;
				}
			}

			ETagMatchType Result = MatchFavorites( SourceItem->Tag );
			switch ( Result ) {
			case ETagMatchType::None: // early return
				continue;
			case ETagMatchType::Parent: // we only get here if option "Show only Defined Facts" is checked
				bNeedsFilteringByText = true;
				break;
			case ETagMatchType::Child: // straight to filtering children, even if this item matched - it is not favorite by itself
				{
					CopyItemIfFavoritesChildrenMatch( SourceItem, OutDestArray, Options );
				}
				break;
			case ETagMatchType::Full:
				bNeedsFilteringByText = true;
				break;
			}

			if ( bNeedsFilteringByText )
			{
				if ( MatchText( SourceItem->Tag.ToString() ) ) // full match by favorites and by search texts
				{
					CopyItem( SourceItem, OutDestArray );
				}
			}
		}
	}

	void FilterMainFactItemChildren( TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		auto MatchText = [ &Options ]( const FString& TagString )
		{
			return MatchSearchBox( Options.SearchBarStrings, TagString ) && MatchSearchToggle( Options.SearchToggleStrings, TagString );
		};
			
		for ( const FFactTreeItemPtr& SourceItem : SourceArray )
		{
			bool bNeedsFilteringByText = false;

			if ( Settings::bShowOnlyDefinedFacts && Options.bIsPlaying )
			{
				if ( SourceItem->Children.Num() > 0 ) // even if this item has value - children can be without value and therefore shoundn't be visible
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
					continue;
				}
				else if ( SourceItem->Value.IsSet() == false )
				{
					continue;
				}
			}

			ETagMatchType Result = MatchFavorites( SourceItem->Tag );
			switch ( Result )
			{
			case ETagMatchType::None:
				bNeedsFilteringByText = true;
				break;
			case ETagMatchType::Parent:
				if ( Settings::bShowFavoritesInMainTree ){	bNeedsFilteringByText = true; }
				else { continue; }
				break;
			case ETagMatchType::Child:
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
				}
				break;
			case ETagMatchType::Full:
				if ( Settings::bShowFavoritesInMainTree ){	bNeedsFilteringByText = true; }
				else { continue; }
				break;
			}

			if ( bNeedsFilteringByText )
			{
				if ( MatchText( SourceItem->Tag.ToString() ) )
				{
					CopyItem( SourceItem, OutDestArray );
				}
				else
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
				}
			}
		}
	}


	void GetLeafTags( TSharedPtr< FGameplayTagNode > Node, TArray< TSharedPtr< FGameplayTagNode > >& OutLeafTagNodes )
	{
		if ( Node->GetChildTagNodes().IsEmpty() )
		{
			OutLeafTagNodes.Add( Node );
			return;
		}

		for ( TSharedPtr< FGameplayTagNode > Child : Node->GetChildTagNodes() )
		{
			GetLeafTags( Child, OutLeafTagNodes );
		}
	};
}


FFactTreeItem::~FFactTreeItem()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsDebuggerModule::Get().TryGetFactSubsystem() )
	{
		FactSubsystem->GetOnFactValueChangedDelegate( Tag ).Remove( Handle );
	}
}

void FFactTreeItem::StartPlay()
{
	Value.Reset();
	InitItem();
}

void FFactTreeItem::EndPlay()
{
	Value.Reset();
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
	(void)OnFactItemValueChanged.ExecuteIfBound( Tag, NewValue );
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

void SFactDebugger::Construct( const FArguments& InArgs )
{
	BuildFactTreeItems();
	
	ChildSlot
	[
		SNew( SVerticalBox )

		// -------------------------------------------------------------------------------------------------------------
		// ToolBar
		+ SVerticalBox::Slot()
		.HAlign( HAlign_Fill )
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Left )
			.FillContentWidth(1.f)
			[
				CreateLeftToolBar()
			]

			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.FillContentWidth( 1.f )
			[
				CreateRightToolBar()
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SSeparator )
		]
		
		// -------------------------------------------------------------------------------------------------------------
		// SearchBar
		+ SVerticalBox::Slot()
		.Padding( 2.f, 0.f )
		.AutoHeight()
		[
			SNew( SHorizontalBox )

			// ---------------------------------------------------------------------------------------------------------
			// Search box
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Fill )
			.VAlign( VAlign_Center )
			.Padding( 0.f, 1.f, 0.f, 1.f )
			[
				SAssignNew( SearchBox, SFactSearchBox )
				.HintText( LOCTEXT( "FactDebugger_SearchHintText", "Search..." ) )
				.ToolTipText( LOCTEXT( "FactDebugger_ToolTipText", "Search facts by tag. You can search by string ('Quest2.Trigger') or by several strings, separated by spaces ('Quest Trigger')\n"
													   "Press Enter to save this text as a toggle" ) )
				.OnTextChanged( this, &SFactDebugger::HandleSearchTextChanged )
				.OnSaveSearchClicked( this, &SFactDebugger::HandleSaveSearchClicked )
			]

			// ---------------------------------------------------------------------------------------------------------
			// Options
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 0.f, 1.f, 0.f, 1.f )
			[
				SAssignNew( OptionsButton, SComboButton )
				.ContentPadding( 4.f )
				.ToolTipText( LOCTEXT( "ShowOptions_ToolTip", "Show options to affect the visibility of items in the Facts Debugger" ) )
				.ComboButtonStyle( FAppStyle::Get(), "SimpleComboButtonWithIcon" ) // Use the tool bar item style for this button
				.OnGetMenuContent( this, &SFactDebugger::HandleGenerateOptionsMenu )
				.HasDownArrow( false )
				.ButtonContent()
				[
					SNew( SImage )
					.Image( FAppStyle::GetBrush( "Icons.Settings") )
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
				.ToolTipText( LOCTEXT( "ClearSearchesButtonToolTip", "Reset all selected toggles") )
				.Visibility_Lambda( [ this ]()
				{
					return IsAnySearchToggleActive() ? EVisibility::Visible : EVisibility::Collapsed;
				} )
				.OnClicked( this, &SFactDebugger::HandleClearTogglesClicked )
				[
					SNew( SImage )
					.Image( FFactDebuggerStyle::Get().GetBrush( "Icons.Reset" ) )
				]
			]

			// ---------------------------------------------------------------------------------------------------------
			// Remove toggles button
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 8.f, 1.f, 2.f, 1.f )
			[
				SNew( SButton )
				.ToolTipText( LOCTEXT( "RemoveSearchesButtonToolTip", "Remove all toggles") )
				.OnClicked_Lambda( [ this ]()
				{
					SearchesContainer->ClearChildren();
					CurrentSearchToggles.Empty();
					FilterItems();

					return FReply::Handled();
				} )
				[
					SNew( SImage )
					.Image( FAppStyle::GetBrush( "Icons.X" ) )
				]
			]
		]
		
		// -------------------------------------------------------------------------------------------------------------
		// Facts trees
		+ SVerticalBox::Slot()
		.FillHeight( 1.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::GetBrush( "Brushes.Panel" ) )
			[
				SAssignNew( Splitter, SSplitter )
				.Orientation( GetDefault< UFactDebuggerSettingsLocal >()->Orientation )

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
							.DecoratorStyleSet( &FFactDebuggerStyle::Get() )
							.AutoWrapText( true )
							.Justification( ETextJustify::Center )
							.Text_Lambda( [ this ]()
							{
								if ( SFactDebugger::FavoriteFacts.IsEmpty() )
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

#if WITH_EDITOR
	TagChangedHandle = UGameplayTagsManager::OnEditorRefreshGameplayTagTree.AddSP( this, &SFactDebugger::RebuildFactTreeItems );
#endif
	FSimpleFactsDebuggerModule::Get().OnGameInstanceStarted.BindRaw( this, &SFactDebugger::HandleGameInstanceStarted );
	FSimpleFactsDebuggerModule::Get().OnGameInstanceEnded.BindRaw( this, &SFactDebugger::HandleGameInstanceEnded );
	if ( InArgs._bIsGameStarted )
	{
		HandleGameInstanceStarted();
	}

	LoadSettings();
	PostFavoritesChanged();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFactDebugger::~SFactDebugger()
{
#if WITH_EDITOR
	UGameplayTagsManager::OnEditorRefreshGameplayTagTree.Remove( TagChangedHandle );
#endif
	FSimpleFactsDebuggerModule::Get().OnGameInstanceStarted.Unbind();
	FSimpleFactsDebuggerModule::Get().OnGameInstanceEnded.Unbind();
}

int32 SFactDebugger::CountAllFilteredItems( FFactTreeItemPtr ParentNode )
{
	int32 Result = 0;
	
	if ( Settings::bShowFavoritesInMainTree == false && SFactDebugger::FavoriteFacts.Contains( ParentNode->Tag ) )
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

	if ( Settings::bShowOnlyLeafFacts && ParentNode->Children.Num() > 0 )
	{
		return Result;
	}

	if ( ParentNode->Tag.IsValid() )
	{
		Result++;
	}

	return Result;
}

int32 SFactDebugger::CountAllFavoriteItems( FFactTreeItemPtr ParentNode, bool bIsParentFavorite )
{
	if ( SFactDebugger::FavoriteFacts.IsEmpty() )
	{
		return 0;
	}

	const UFactDebuggerSettingsLocal* Settings = GetDefault< UFactDebuggerSettingsLocal >();
	int32 Result = 0;
	
	if ( bIsParentFavorite == false && SFactDebugger::FavoriteFacts.Contains( ParentNode->Tag ) )
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

	if ( Settings::bShowOnlyLeafFacts && ParentNode->Children.Num() > 0 )
	{
		return Result;
	}
	
	if ( ParentNode->Tag.IsValid() && ( bIsParentFavorite || bHasFavoriteChild ) )
	{
		Result++;
	}

	return Result;
}

void SFactDebugger::HandleGameInstanceStarted()
{
	bIsPlaying = true;
	if ( Settings::bShowOnlyDefinedFacts )
	{
		FilterItems();
	}
	InitItem( RootItem );
	InitItem( FilteredRootItem );
	InitItem( FavoritesRootItem );
}

void SFactDebugger::HandleGameInstanceEnded()
{
	bIsPlaying = false;
	if ( Settings::bShowOnlyDefinedFacts )
	{
		FilterItems();
	}

	ResetItem( RootItem );
	ResetItem( FilteredRootItem );
	ResetItem( FavoritesRootItem );
}

void SFactDebugger::InitItem( FFactTreeItemPtr Item )
{
	Item->StartPlay();
	for ( FFactTreeItemPtr& ChildItem : Item->Children )
	{
		InitItem( ChildItem );
	}
}

void SFactDebugger::ResetItem( FFactTreeItemPtr Item )
{
	Item->EndPlay();
	for ( FFactTreeItemPtr& ChildItem : Item->Children )
	{
		ResetItem( ChildItem );
	}
}

TSharedRef< SWidget > SFactDebugger::CreateLeftToolBar()
{
	FSlimHorizontalToolBarBuilder Toolbar( nullptr, FMultiBoxCustomization::None );
	Toolbar.BeginSection( "Options" );
	{
		Toolbar.AddToolBarButton(
		FUIAction(
		FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowFavoritesInMainTree ) ),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda( [](){ return Settings::bShowFavoritesInMainTree; })
			),
			NAME_None,
			TAttribute< FText >(),
			LOCTEXT( "Options_ShowFavorites_ToolTip", "Show Favorites in Main Tree also" ),
			FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "Icons.Star.OutlineFilled" ),
			EUserInterfaceActionType::ToggleButton
		);
		
		Toolbar.AddToolBarButton(
		FUIAction(
		FExecuteAction::CreateLambda( [ this ]()
			{
				Settings::bShowOnlyLeafFacts = !Settings::bShowOnlyLeafFacts;
				SaveSettings();

				BuildFactTreeItems();
				PostFavoritesChanged();
			} ),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda( [](){ return Settings::bShowOnlyLeafFacts; })
			),
			NAME_None,
			TAttribute< FText >(),
			LOCTEXT( "Options_ShowLeafs_ToolTip", "Show only leaf Facts in each tree as a list.\nNote: Facts with child tags will not be show in the trees, even if they have defined values.\n" ),
			FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "Icons.LeafFacts" ),
			EUserInterfaceActionType::ToggleButton
		);

		Toolbar.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowOnlyDefinedFacts ) ),
			FCanExecuteAction(),
			FIsActionChecked::CreateLambda( [](){ return Settings::bShowOnlyDefinedFacts; })
			),
			NAME_None,
			TAttribute< FText >(),
			LOCTEXT( "Options_ShowDefined_ToolTip", "Show only defined Facts with values" ),
			FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "Icons.DefinedFacts" ),
			EUserInterfaceActionType::ToggleButton,
			NAME_None,
			TAttribute< EVisibility >::CreateLambda( [ this ]() { return bIsPlaying ? EVisibility::Visible : EVisibility::Hidden; } )
		);
	}
	Toolbar.EndSection();

	return Toolbar.MakeWidget();
}

TSharedRef<SWidget> SFactDebugger::CreateRightToolBar()
{
	FSlimHorizontalToolBarBuilder Toolbar( nullptr, FMultiBoxCustomization::None );
	Toolbar.BeginSection( "Options" );
	{
		Toolbar.AddComboButton(
			FUIAction(),
			FOnGetContent::CreateSP( this, &SFactDebugger::HandleGeneratePresetsMenu ),
			LOCTEXT( "PresetsButton", "Presets" ),
			LOCTEXT( "PresetsButton_Toolpit", "Open menu to load FactPresets" ),
			FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "ClassIcon.FactPreset" ),
			false,
			NAME_None,
			TAttribute< EVisibility >::CreateLambda( [ this ]() { return bIsPlaying ? EVisibility::Visible : EVisibility::Hidden; } )
		);
	}
	Toolbar.EndSection();

	return Toolbar.MakeWidget();
}

TSharedRef< SWidget > SFactDebugger::CreateTreeLabel( const FText& InLabel ) const
{
	return SNew( SBorder )
		.BorderImage( FAppStyle::GetBrush( "Brushes.Background" ) )
		.Padding( 0.f, 0.f, 0.f, 3.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::GetBrush( "Brushes.Header" ) )
			.Padding( 10.f, 4.f )
			[
				SNew( STextBlock )
				.Text( InLabel )
				.TextStyle( FAppStyle::Get(), "ButtonText" )
				.Font( FAppStyle::GetFontStyle( "NormalFontBold" ) )
			]	
		];
}

TSharedRef< SWidget > SFactDebugger::CreateFactsTree( bool bIsFavoritesTree )
{
	TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;
	FFactTreeItemPtr& ItemsSource = bIsFavoritesTree ? FavoritesRootItem : FilteredRootItem;
	
	return SAssignNew( TreeView, SFactsTreeView )
		.TreeItemsSource( &ItemsSource->Children )
		.OnGenerateRow( this, &SFactDebugger::OnGenerateWidgetForFactsTreeView )
		.OnGetChildren( this, &SFactDebugger::OnGetChildren )
		.OnExpansionChanged( this, &SFactDebugger::HandleExpansionChanged, false, bIsFavoritesTree )
		.OnSetExpansionRecursive( this, &SFactDebugger::HandleExpansionChanged, true, bIsFavoritesTree )
		.OnGeneratePinnedRow( this, &SFactDebugger::HandleGeneratePinnedTreeRow )
		.ShouldStackHierarchyHeaders_Lambda( []() { return Settings::bShouldStackHierarchyHeaders; } )
		.OnContextMenuOpening_Lambda( [ this, bIsFavoritesTree ]()
		{
			return bIsFavoritesTree ? HandleGenerateFavoritesContextMenu() : HandleGenerateMainContextMenu();
		} )
		.SelectionMode( ESelectionMode::Type::Single )
		.HeaderRow
		(
			CreateHeaderRow( bIsFavoritesTree )
		);
}

TSharedRef< SHeaderRow > SFactDebugger::CreateHeaderRow( bool bIsFavoritesTree ) const
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
					? FAppStyle::GetBrush( "Icons.Star" )
					: Settings::bShowFavoritesInMainTree
						? FFactDebuggerStyle::Get().GetBrush( "Icons.Star.OutlineFilled" )
						: FFactDebuggerStyle::Get().GetBrush( "Icons.Star.Outline" );
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

TSharedRef< SWidget > SFactDebugger::CreateFilterStatusWidget( bool bIsFavoritesTree ) const
{
	return SNew( SBorder )
		.BorderImage( FAppStyle::GetBrush( "Brushes.Header" ) )
		.VAlign( VAlign_Center )
		.HAlign( HAlign_Left )
		.Padding( 10.f, 4.f )
		[
			SNew( STextBlock )
			.Text( this, &SFactDebugger::GetFilterStatusText, bIsFavoritesTree )
			.ColorAndOpacity( this, &SFactDebugger::GetFilterStatusTextColor, bIsFavoritesTree )
		];
}

TSharedRef< ITableRow > SFactDebugger::OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr InItem, const TSharedRef< STableViewBase >& TableViewBase )
{
	class SFactTreeItem : public SMultiColumnTableRow< FFactTreeItemPtr >
	{
	public:
		SLATE_BEGIN_ARGS( SFactTreeItem ) {}
		SLATE_END_ARGS()

		SFactTreeItem()
			: FavoriteBrush( FAppStyle::GetBrush( "Icons.Star" ) )
			, NormalBrush( FFactDebuggerStyle::Get().GetBrush( "Icons.Star.Outline" ) )
			, EvenColor( USlateThemeManager::Get().GetColor( EStyleColor::Recessed ) )
			, OddColor( USlateThemeManager::Get().GetColor( EStyleColor::Background ) )
			, Animation( FCurveSequence(0.f, AnimationDuration ) )
			, AnimationColor( FFactDebuggerStyle::Get().GetColor( "Colors.FactChanged" ) )
		{ }

		void Construct( const FArguments& InArgs, const TSharedRef< STableViewBase > InOwnerTable, TSharedPtr< SFactDebugger > InFactDebugger, FFactTreeItemPtr InItem )
		{
			Item = InItem;
			FactDebugger = InFactDebugger;

			SMultiColumnTableRow::Construct( FSuperRowType::FArguments()
				.Style( FAppStyle::Get(), "TableView.AlternatingRow" ), InOwnerTable );

			Item->OnFactItemValueChanged.BindRaw( this, &SFactTreeItem::HandleItemValueChanged );
			TryPlayAnimation();
		}

		virtual void ResetRow() override
		{
			Item->OnFactItemValueChanged.Unbind();
		}
		
		virtual TSharedRef< SWidget > GenerateWidgetForColumn( const FName& InColumnName ) override
		{
			if ( InColumnName == "Favorites" )
			{
				return SNew( SButton )
					.ButtonStyle( FAppStyle::Get(), "NoBorder" )
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
						SNew( SFactExpanderArrow, SharedThis( this ) )
					]
					
					+SHorizontalBox::Slot()
					.FillWidth( 1.f )
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.ColorAndOpacity( Item->Tag.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground() )
						.Text_Lambda( [ this ]()
						{
							return FText::FromString( Settings::bShowFullFactNames ? Item->Tag.ToString() : Item->SimpleTagName.ToString() );
						} )
						.HighlightText_Lambda( [ this ](){ return FactDebugger->CurrentSearchText; } )
					];
			}
			else if ( InColumnName == "Value" )
			{
				return SNew( SBox )
					.Padding( 1.f )
					[
						SNew( SNumericEntryBox< int32 > )
						.Value( TAttribute< TOptional< int32 > >::CreateSP( this, &SFactTreeItem::GetItemValue ) )
						.OnValueCommitted( FOnInt32ValueCommitted::CreateRaw( Item.Get(), &FFactTreeItem::HandleNewValueCommited ) )
						.UndeterminedString( LOCTEXT( "FactUndefinedValue", "undefined" ) )
						.IsEnabled_Lambda( [ this ]() { return FactDebugger->bIsPlaying; } )
					];
			}
			else
			{
				return SNew( STextBlock ).Text( LOCTEXT( "UnknownColumn", "Unknown Column" ) );
			}
		}
		
		FReply HandleFavoriteClicked()
		{
			check( Item.IsValid() );
			if ( SFactDebugger::FavoriteFacts.Contains( Item->Tag ) )
			{
				SFactDebugger::FavoriteFacts.Remove( Item->Tag );
			}
			else
			{
				SFactDebugger::FavoriteFacts.Add( Item->Tag );
			}
			
			FactDebugger->SaveSettings();
			FactDebugger->PostFavoritesChanged();
			return FReply::Handled();
		}

		void HandleItemValueChanged( FFactTag FactTag, int32 NewValue )
		{
			Animation.Play( AsShared() );
		}

		void TryPlayAnimation()
		{
			if ( Animation.IsPlaying() )
			{
				return;
			}
			
			float CurrentTime = FSlateApplication::Get().GetCurrentTime();
			float AnimStartTime = CurrentTime - Item->ValueChangedTime;
			if ( AnimStartTime < AnimationDuration )
			{
				Animation.Play( AsShared(), false, AnimStartTime );
			}
		}

		const virtual FSlateBrush* GetBorder() const override
		{
			if ( Animation.IsPlaying() )
			{
				const bool bEvenEntryIndex = ( IndexInList % 2 == 0 );
				
				ChangedBrush.TintColor = FMath::Lerp( bEvenEntryIndex ? EvenColor : OddColor, AnimationColor, 1 - Animation.GetLerp() );
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
			check( Item.IsValid() );
			const bool bIsSelected = FactDebugger->FactsTreeView->IsItemSelected( Item.ToSharedRef() );
		
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
			check( Item.IsValid() );

			for ( FFactTag& FavoriteFact : SFactDebugger::FavoriteFacts )
			{
				if ( FavoriteFact == Item->Tag )
				{
					return true;
				}
			}

			return false;
		}

		TOptional< int32 > GetItemValue() const
		{
			check( Item.IsValid() );
			return Item->Value;
		}

	private:

		FFactTreeItemPtr Item;
		TSharedPtr< SFactDebugger > FactDebugger;

		const FSlateBrush* FavoriteBrush = nullptr;
		const FSlateBrush* NormalBrush = nullptr;
		mutable FSlateColorBrush ChangedBrush = FSlateColorBrush( FStyleColors::Background );
		
		FLinearColor EvenColor;
		FLinearColor OddColor;

		const float AnimationDuration = .8f;
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

TSharedRef< ITableRow > SFactDebugger::HandleGeneratePinnedTreeRow( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase )
{
	return SNew( STableRow< TSharedPtr< FString > >, TableViewBase )
		[
			SNew( SBox )
			.HeightOverride( 22.f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text_Lambda( [ FactTreeItem ]()
				{
					return FText::FromString( Settings::bShowFullFactNames ? FactTreeItem->Tag.ToString() : FactTreeItem->SimpleTagName.ToString() );
				} )
			]
		];
}

void SFactDebugger::OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray<FFactTreeItemPtr>& Children )
{
	if ( FactTreeItem.IsValid() )
	{
		Children.Append( FactTreeItem->Children );
	}
}

void SFactDebugger::HandleExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded, bool bRecursive, bool bIsFavoritesTree )
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

FText SFactDebugger::GetFilterStatusText( bool bIsFavoritesTree ) const
{
	int32 AllFactsCount = bIsFavoritesTree ? Utils::AllFavoriteFactsCount : Utils::AllFilteredFactsCount;
	int32 CurrentFactCount = bIsFavoritesTree ? Utils::CurrentFavoriteFactsCount : Utils::CurrentFilteredFactsCount;
	const TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;

	if ( CurrentSearchText.IsEmpty() && IsAnySearchToggleActive() == false && ( Settings::bShowOnlyDefinedFacts == false || bIsPlaying == false ) )
	{
		return FText::Format( LOCTEXT( "ShowingAllFacts", "{0} facts" ), FText::AsNumber( AllFactsCount ) );
	}

	if ( TreeView->GetRootItems().IsEmpty() )
	{
		return FText::Format( LOCTEXT( "NoMatchingFacts", "No matching facts ({0} total)" ), FText::AsNumber( AllFactsCount ) );
	}
	
	return FText::Format( LOCTEXT( "ShowingFilteredFacts", "{0} facts ({1} total)" ), FText::AsNumber( CurrentFactCount ), FText::AsNumber( AllFactsCount ) );

}

FSlateColor SFactDebugger::GetFilterStatusTextColor( bool bIsFavoritesTree ) const
{
	const TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;

	if ( CurrentSearchText.IsEmpty() && IsAnySearchToggleActive() == false && ( Settings::bShowOnlyDefinedFacts == false || bIsPlaying == false ) )
	{
		return FSlateColor::UseForeground();
	}

	if ( TreeView->GetRootItems().IsEmpty() )
	{
		return FStyleColors::AccentRed;
	}

	return FStyleColors::AccentGreen;
}

TSharedRef<SWidget> SFactDebugger::HandleGeneratePresetsMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };
	MenuBuilder.SetSearchable( false );

	MenuBuilder.BeginSection( NAME_None, LOCTEXT( "LoadPreset_MenuSection", "Load preset" ));
	{
		TArray< FAssetData > AssetData;
		IAssetRegistry::Get()->GetAssetsByClass( UFactPreset::StaticClass()->GetClassPathName(), AssetData );

		TSharedPtr< SFactPresetPicker > PresetPicker;
		TSharedRef< SWidget > MenuWidget = SNew( SBox )
			.WidthOverride( 300.f )
			.HeightOverride( 300.f )
			.Padding( 2.f )
			[
				SAssignNew( PresetPicker, SFactPresetPicker, AssetData )
				.OnPresetSelected_Lambda( [ this ]( const UFactPreset* Preset )
				{
					FSimpleFactsDebuggerModule::Get().LoadFactPreset( Preset );
					FSlateApplication::Get().DismissAllMenus();
				})
			];

		// Set focus to the search box on creation
		FSlateApplication::Get().SetKeyboardFocus( PresetPicker->GetWidgetToFocusOnOpen() );
		FSlateApplication::Get().SetUserFocus( 0, PresetPicker->GetWidgetToFocusOnOpen() );

		MenuBuilder.AddWidget( MenuWidget, FText(), true, false );
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget(  );
}

TSharedRef< SWidget > SFactDebugger::HandleGenerateOptionsMenu()
{
	FMenuBuilder MenuBuilder( false, nullptr );

	MenuBuilder.BeginSection( "", LOCTEXT( "Options_HierarchySectionName", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ExpandAll", "Expand All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactDebugger::HandleExpandAllClicked ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_CollapseAll", "Collapse All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactDebugger::HandleCollapseAllClicked ) )
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowHierarchy", "Stack Hierarchy Headers" ),
			LOCTEXT( "Options_ShowHierarchy_ToolTip", "Toggle pinning of the hierarchy of items at the top of the outliner" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShouldStackHierarchyHeaders ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return Settings::bShouldStackHierarchyHeaders; })
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
				FIsActionChecked::CreateLambda( [](){ return Settings::bShowRootFactTag; })
				),
			NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowFullNames", "Show Full Fact Names" ),
			LOCTEXT( "Options_ShowFullNames_ToolTip", "Show Full Fact Names" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( [ this ]()
				{
					Settings::bShowFullFactNames = !Settings::bShowFullFactNames;
					SaveSettings();
				} ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return Settings::bShowFullFactNames; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowFavorites", "Show Favorites in Main Tree" ),
			LOCTEXT( "Options_ShowFavorites_ToolTip", "Show Favorites in Main Tree also" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowFavoritesInMainTree ) ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return Settings::bShowFavoritesInMainTree; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowLeafs", "Show Only Leaf Facts" ),
			LOCTEXT( "Options_ShowLeafs_ToolTip", "Show only leaf Facts in each tree as a list.\nNote: if some of defined Facts have child tags, they will not be shown in the trees.\n" ),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateLambda( [ this ]()
				{
					Settings::bShowOnlyLeafFacts = !Settings::bShowOnlyLeafFacts;
					SaveSettings();

					BuildFactTreeItems();
					PostFavoritesChanged();
				} ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return Settings::bShowOnlyLeafFacts; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowDefined", "Show only Defined Facts" ),
			LOCTEXT( "Options_ShowDefined_ToolTip", "Show only defined Facts with values" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( TOGGLE_FACT_SETTING( bShowOnlyDefinedFacts ) ),
				FCanExecuteAction::CreateLambda( [ this ]()	{ return bIsPlaying; } ),
				FIsActionChecked::CreateLambda( [](){ return Settings::bShowOnlyDefinedFacts; })
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
			FExecuteAction::CreateRaw( this, &SFactDebugger::HandleOrientationChanged, Orient_Horizontal ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactDebuggerSettingsLocal >()->Orientation == Orient_Horizontal; })
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_Horizontal", "Vertical" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateRaw( this, &SFactDebugger::HandleOrientationChanged, Orient_Vertical ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactDebuggerSettingsLocal >()->Orientation == Orient_Vertical; })
				),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget();
}

void SFactDebugger::GenerateCommonContextMenu( FMenuBuilder& MenuBuilder, bool bIsFavoritesTree )
{
	TSharedPtr< SFactsTreeView >& TreeView = bIsFavoritesTree ? FavoriteFactsTreeView : FactsTreeView;
	TArray< FFactTreeItemPtr >& FactItems = bIsFavoritesTree ? FavoritesRootItem->Children : FilteredRootItem->Children;
	
	MenuBuilder.BeginSection( "", LOCTEXT( "ContextMenu_TreeSection", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_ExpandTree", "Expand Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactDebugger::SetItemsExpansion, TreeView, FactItems, true, true ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_CollapseTree", "Collapse Tree" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactDebugger::SetItemsExpansion, TreeView, FactItems, false, true ) )
		);
	}
	MenuBuilder.EndSection();
}

TSharedPtr< SWidget > SFactDebugger::HandleGenerateMainContextMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };

	GenerateCommonContextMenu( MenuBuilder, false );

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SWidget> SFactDebugger::HandleGenerateFavoritesContextMenu()
{
	FMenuBuilder MenuBuilder{ true, nullptr };

	GenerateCommonContextMenu( MenuBuilder, true );

	MenuBuilder.BeginSection( "", LOCTEXT( "ContextMenu_FavoritesSection", "Favorites" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "ContextMenu_RemoveAllFavorites", "Remove All Favorites" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda( [ this ]()
				{
					SFactDebugger::FavoriteFacts.Reset();
					SaveSettings();
					PostFavoritesChanged();
				} ),
				FCanExecuteAction::CreateLambda( [ this ]()
				{
					return Utils::AllFavoriteFactsCount > 0;
				} )
			)
		);

		TArray< FFactTreeItemPtr > SelectedItems = FavoriteFactsTreeView->GetSelectedItems();
		if ( SelectedItems.Num() == 1 && HasFavoritesRecursive( SelectedItems[ 0 ] ) )
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT( "ContextMenu_RemoveItemFavorites", "Remove All Favorites for Item" ),
				LOCTEXT( "ContextMenu_RemoveItemFavorites_ToolTip", "Remove all children facts that are Favorites (including this item)" ),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateLambda( [ this ]()
					{
						TArray< FFactTreeItemPtr > SelectedItems = FavoriteFactsTreeView->GetSelectedItems();
						ClearFavoritesRecursive( SelectedItems[ 0 ] );
						SaveSettings();
						PostFavoritesChanged();
					} )
				)
			);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SFactDebugger::ClearFavoritesRecursive( FFactTreeItemPtr Item )
{
	if ( SFactDebugger::FavoriteFacts.Contains( Item->Tag ) )
	{
		SFactDebugger::FavoriteFacts.Remove( Item->Tag );
	}

	for ( FFactTreeItemPtr Child : Item->Children )
	{
		ClearFavoritesRecursive( Child );
	}
}

bool SFactDebugger::HasFavoritesRecursive( FFactTreeItemPtr Item )
{
	if ( SFactDebugger::FavoriteFacts.Contains( Item->Tag ) )
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

void SFactDebugger::PostFavoritesChanged()
{
	Utils::AllFilteredFactsCount = CountAllFilteredItems( RootItem );
	Utils::AllFavoriteFactsCount = CountAllFavoriteItems( RootItem, false );

	FilterItems();
}

void SFactDebugger::HandleSearchTextChanged( const FText& SearchText )
{
	CurrentSearchText = SearchText;
	FilterItems();
}

void SFactDebugger::HandleSaveSearchClicked( const FText& SearchText )
{
	if ( SearchText.IsEmpty() )
	{
		return;
	}

	TArray< FString > ExistingStrings;
	ExistingStrings.Reserve( CurrentSearchToggles.Num() );

	for ( const SFactSearchToggleRef& SearchToggle : CurrentSearchToggles )
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

	SFactSearchToggleRef NewSearchToggle = ConstructSearchToggle( SearchText, true );
	CurrentSearchToggles.Add( NewSearchToggle );

	RefreshSearchToggles();
	SearchBox->SetText( FText::GetEmpty() );

	SaveSettings();
}

void SFactDebugger::FilterItems()
{
	FilteredRootItem.Reset();
	FavoritesRootItem.Reset();

	// Parse filter strings
	TArray< FString > ActiveTogglesText;
	for ( const SFactSearchToggleRef SearchToggle : CurrentSearchToggles )
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
	Utils::FFilterOptions Options{ ActiveTogglesText, Tokens, bIsPlaying };
	Utils::FilterMainFactItemChildren( RootItem->Children, FilteredRootItem->Children, Options );
	Utils::FilterFavoritesFactItemChildren( RootItem->Children, FavoritesRootItem->Children,Options );

	Utils::CurrentFilteredFactsCount = CountAllFilteredItems( FilteredRootItem );
	Utils::CurrentFavoriteFactsCount = CountAllFavoriteItems( FavoritesRootItem, false );
	
	FactsTreeView->SetTreeItemsSource( &FilteredRootItem->Children );
	FavoriteFactsTreeView->SetTreeItemsSource( &FavoritesRootItem->Children );

	if ( ( Settings::bShowOnlyDefinedFacts && bIsPlaying ) || ActiveTogglesText.Num() || Tokens.Num() )
	{
		SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true, false );
		SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true, false );
	}
	else
	{
		// RestoreExpansionState();
		SetDefaultItemsExpansion( FactsTreeView, FilteredRootItem->Children, SFactDebugger::MainExpandedFacts );
		SetDefaultItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, SFactDebugger::FavoritesExpandedFacts );
	}

	FactsTreeView->RequestTreeRefresh();
	FavoriteFactsTreeView->RequestTreeRefresh();
}

void SFactDebugger::HandleExpandAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true, true );
	OptionsButton->SetIsOpen( false );
}

void SFactDebugger::HandleCollapseAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, false, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, false, true );
	OptionsButton->SetIsOpen( false );
}

void SFactDebugger::SetItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, TArray<FFactTreeItemPtr> FactItems, bool bShouldExpand, bool bPersistExpansion )
{
	TGuardValue< bool > PersistExpansionChangeGuard( bPersistExpansionChange, bPersistExpansion );

	for ( const FFactTreeItemPtr& Item : FactItems )
	{
		TreeView->SetItemExpansion( Item, bShouldExpand );
		SetItemsExpansion( TreeView, Item->Children, bShouldExpand, bPersistExpansion );
	}
}

void SFactDebugger::RestoreExpansionState()
{
	TGuardValue< bool > PersistExpansionChangeGuard( bPersistExpansionChange, false );
	
	const UFactDebuggerSettingsLocal* Settings = GetDefault< UFactDebuggerSettingsLocal >();

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

void SFactDebugger::SetDefaultItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, const TArray< FFactTreeItemPtr >& FactItems, const TSet< FFactTag >& ExpandedFacts )
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

bool SFactDebugger::FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag,
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

void SFactDebugger::CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates )
{
	for ( FSearchToggleState& ToggleState : SearchToggleStates )
	{
		SFactSearchToggleRef NewSearchToggle = ConstructSearchToggle( ToggleState.SearchText, ToggleState.bIsToggleChecked );
		CurrentSearchToggles.Add( NewSearchToggle );
	}

	RefreshSearchToggles();
}

SFactSearchToggleRef SFactDebugger::ConstructSearchToggle( const FText& InSearchText, bool bInChecked )
{
	return SNew( SFactSearchToggle, InSearchText )
			.OnClickedOnce( this, &SFactDebugger::HandleSearchToggleClicked )
			.OnRightButtonClicked( this, &SFactDebugger::HandleRemoveSearchToggle )
			.OnAltClicked( this, &SFactDebugger::HandleRemoveSearchToggle )
			.IsToggleChecked( bInChecked );
}

FReply SFactDebugger::HandleRemoveSearchToggle()
{
	CleanupSearchesMarkedForDelete();
	RefreshSearchToggles();
	FilterItems();
	SaveSettings();

	return FReply::Handled();
}

void SFactDebugger::CleanupSearchesMarkedForDelete()
{
	CurrentSearchToggles.RemoveAllSwap( []( const SFactSearchToggleRef& SearchToggle )
	{
		return SearchToggle->GetIsMarkedForDelete();
	} );
}

void SFactDebugger::RefreshSearchToggles()
{
	SearchesContainer->ClearChildren();
	
	for ( const SFactSearchToggleRef& SearchToggle: CurrentSearchToggles )
	{
		SearchesContainer->AddSlot()
		[
			SearchToggle
		];
	}
}

FReply SFactDebugger::HandleClearTogglesClicked()
{
	for ( const SFactSearchToggleRef& SearchToggle : CurrentSearchToggles )
	{
		SearchToggle->SetIsButtonChecked( false );
	}

	FilterItems();
	SaveSettings();

	return FReply::Handled();
}

FReply SFactDebugger::HandleSearchToggleClicked()
{
	FilterItems();
	SaveSettings();

	return FReply::Handled();
}

TArray<FSearchToggleState> SFactDebugger::GetSearchToggleStates()
{
	TArray< FSearchToggleState > SearchToggleStates;
	for ( const SFactSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		SearchToggleStates.Emplace( SearchToggle->GetIsToggleChecked(), SearchToggle->GetSearchText() );
	}

	return SearchToggleStates;
}

bool SFactDebugger::IsAnySearchToggleActive() const
{
	for ( const SFactSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		if ( SearchToggle->GetIsToggleChecked() )
		{
			return true;
		}
	}

	return false;
}

void SFactDebugger::BuildFactTreeItems()
{
	RootItem = MakeShared< FFactTreeItem >();

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	if ( Settings::bShowOnlyLeafFacts )
	{
		TArray < TSharedPtr< FGameplayTagNode > > LeafTagNodes;
		TSharedPtr< FGameplayTagNode > Node = Manager.FindTagNode( FFactTag::GetRootTag() );
		Utils::GetLeafTags( Node, LeafTagNodes );
		
		for ( TSharedPtr< FGameplayTagNode >& ChildNode : LeafTagNodes )
		{
			BuildFactItem( RootItem, ChildNode );
		}
	}
	else
	{
		TSharedPtr< FGameplayTagNode > Node = Manager.FindTagNode( FFactTag::GetRootTag() );

		if ( Settings::bShowRootFactTag )
		{
			BuildFactItem( RootItem, Node );
		}
		else
		{
			for ( TSharedPtr< FGameplayTagNode >& ChildNode : Node->GetChildTagNodes() )
			{
				BuildFactItem( RootItem, ChildNode );
			}
		}
	}

	FilteredRootItem = RootItem;
	FavoritesRootItem = MakeShared< FFactTreeItem >();
}

FFactTreeItemPtr SFactDebugger::BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode )
{
	FFactTreeItemPtr ThisItem = MakeShared< FFactTreeItem >(  );
	ThisItem->Tag = FFactTag::ConvertChecked( ThisNode->GetCompleteTag() );
	ThisItem->SimpleTagName = ThisNode->GetSimpleTagName();
	ThisItem->Children.Reserve( ThisNode->GetChildTagNodes().Num() );
	ThisItem->InitItem();
	ThisItem->OnFactItemValueChanged.BindSP( this, &SFactDebugger::HandleFactValueChanged );
	
	ParentNode->Children.Add( ThisItem );
	
	for ( TSharedPtr< FGameplayTagNode > Node : ThisNode->GetChildTagNodes() )
	{
		BuildFactItem( ThisItem, Node );
	}

	return ThisItem;
}

void SFactDebugger::RebuildFactTreeItems()
{
	BuildFactTreeItems();
	FilterItems();
}

void SFactDebugger::HandleFactValueChanged( FFactTag FactTag, int32 NewValue )
{
	check( bIsPlaying );
	
	// only do this if trees are displaying only defined facts, otherwise changed items already somewhere in trees (or explicitly filtered out)
	if ( Settings::bShowOnlyDefinedFacts == false ) 
	{
		return;
	}

	// if item is in some tree - skip
	TArray< FFactTreeItemPtr > Path;
	if ( FindItemByTagRecursive( FavoritesRootItem, FactTag, Path ) )
	{
		return;
	}
	if ( FindItemByTagRecursive( FilteredRootItem, FactTag, Path ) )
	{
		return;
	}

	FindItemByTagRecursive( RootItem, FactTag, Path );
	check( Path.Num() );
	Path.Last()->ValueChangedTime = FSlateApplication::Get().GetCurrentTime();

	// we need to filter items, in order for element to appear in trees (or not, if it will not pass filters)
	FilterItems();
}


void SFactDebugger::LoadSettings()
{
	UFactDebuggerSettingsLocal* SettingsLocal = GetMutableDefault< UFactDebuggerSettingsLocal >();
	Settings::bShowRootFactTag = SettingsLocal->bShowRootFactTag;
	Settings::bShowFullFactNames = SettingsLocal->bShowFullFactNames;
	Settings::bShouldStackHierarchyHeaders = SettingsLocal->bShouldStackHierarchyHeaders;
	Settings::bShowFavoritesInMainTree = SettingsLocal->bShowFavoritesInMainTree;
	Settings::bShowOnlyLeafFacts = SettingsLocal->bShowOnlyLeafFacts;
	Settings::bShowOnlyDefinedFacts = SettingsLocal->bShowOnlyDefinedFacts;
	
	CreateDefaultSearchToggles( SettingsLocal->ToggleStates );
	SFactDebugger::FavoriteFacts = SettingsLocal->FavoriteFacts;
}

void SFactDebugger::SaveSettings()
{
	UFactDebuggerSettingsLocal* SettingsLocal = GetMutableDefault< UFactDebuggerSettingsLocal >();
	SettingsLocal->bShowRootFactTag = Settings::bShowRootFactTag;
	SettingsLocal->bShowFullFactNames = Settings::bShowFullFactNames;
	SettingsLocal->bShouldStackHierarchyHeaders = Settings::bShouldStackHierarchyHeaders;
	SettingsLocal->bShowFavoritesInMainTree = Settings::bShowFavoritesInMainTree;
	SettingsLocal->bShowOnlyLeafFacts = Settings::bShowOnlyLeafFacts;
	SettingsLocal->bShowOnlyDefinedFacts = Settings::bShowOnlyDefinedFacts;
	
	SettingsLocal->ToggleStates = GetSearchToggleStates();
	SettingsLocal->FavoriteFacts = SFactDebugger::FavoriteFacts;
	
	SettingsLocal->SaveConfig();
}

void SFactDebugger::HandleOrientationChanged( EOrientation Orientation )
{
	UFactDebuggerSettingsLocal* Settings = GetMutableDefault< UFactDebuggerSettingsLocal >();
	Settings->Orientation = Orientation;
	Settings->SaveConfig();
	
	Splitter->SetOrientation( Orientation );
}

#undef LOCTEXT_NAMESPACE