// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditor.h"

#include "ContentBrowserModule.h"
#include "FactsDebuggerSettingsLocal.h"
#include "FactsEditorStyle.h"
#include "FactsPreset.h"
#include "FactSubsystem.h"
#include "IDocumentation.h"
#include "SFactsEditorSearchToggle.h"
#include "SFactsPresetPicker.h"
#include "SimpleFactsEditor.h"
#include "SlateOptMacros.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "FactsEditor"

TArray< FFactTag > SFactsEditor::CollapsedStates;
TArray< FFactTag > SFactsEditor::FavoriteFacts;

namespace Utils
{
	struct FFilterOptions
	{
		TArray< FString > SearchToggleStrings;
		TArray< FString > SearchBarStrings;
		bool bFilterFavorites;
		
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
			TArray<FString> Tokens;
			SearchString.ParseIntoArray( Tokens, TEXT("&") );

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
		for ( FFactTag FavoriteFact : SFactsEditor::FavoriteFacts )
		{
			if ( CheckedTag == FavoriteFact )
			{
				return ETagMatchResult::Full;
			}
			if ( FavoriteFact.MatchesTag( CheckedTag ) )
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

			switch (Result) {
			case ETagMatchResult::None: // Fact and it's parent tags is not in favorites
				{
					if ( Options.bFilterFavorites == false )
					{
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, MatchText( SourceItem->Tag.ToString() ) == false );
					}
				}
				break;
			case ETagMatchResult::Partial: // Fact is not in favorites, but one of the parent tags is 
				{
					bool bShouldFilterChildren = Options.bFilterFavorites || ( Settings->bRemoveFavoritesFromMainTree || MatchText( SourceItem->Tag.ToString() ) == false );
					Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, bShouldFilterChildren );
				}
				break;
			case ETagMatchResult::Full: // Fact is favorite
				{
					if ( Options.bFilterFavorites || Settings->bRemoveFavoritesFromMainTree == false )
					{
						Utils::CopyMatchedItem( SourceItem, OutDestArray, Options, MatchText( SourceItem->Tag.ToString() ) == false );
					}
				}
				break;
			}
		}
	}

}


FFactTreeItem::~FFactTreeItem()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsEditorModule::Get().TryGetFactSubsystem() )
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
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsEditorModule::Get().TryGetFactSubsystem() )
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
}

void FFactTreeItem::HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type )
{
	if ( Type == ETextCommit::Default || Type == ETextCommit::OnCleared )
	{
		return;
	}
	
	Value = NewValue;
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsEditorModule::Get().TryGetFactSubsystem() )
	{
		FactSubsystem->ChangeFactValue( Tag, NewValue, EFactValueChangeType::Set );
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactsEditor::Construct( const FArguments& InArgs )
{
	BuildFactTreeItems();

	ChildSlot
	[
		SNew(SVerticalBox)

		// -------------------------------------------------------------------------------------------------------------
		// Presets menu

		+ SVerticalBox::Slot()
		.Padding( 2.f )
		.HAlign( HAlign_Right )
		.AutoHeight()
		[
			SAssignNew( ComboButton, SComboButton )
			.ToolTipText( LOCTEXT( "PresetsButton_Toolpit", "Open presets menu" ) )
			.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton"))
			.OnGetMenuContent( this, &SFactsEditor::HandleGeneratePresetsMenu )
			.ForegroundColor( FStyleColors::Foreground )
			.ButtonContent()
			[
				SNew(SHorizontalBox)

				// -------------------------------------------------------------------------------------------------------------
				// Preset icon
				+ SHorizontalBox::Slot()
				.Padding(0, 1, 4, 0)
				.AutoWidth()
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("AssetEditor.SaveAsset"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]

				// -------------------------------------------------------------------------------------------------------------
				// Preset text
				+ SHorizontalBox::Slot()
				.Padding(0, 1, 0, 0)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PresetsButton", "Presets"))
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

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(0.f, 1.f, 0.f, 1.f)
			[
				SAssignNew( SearchBox, SSearchBox )
				.HintText(LOCTEXT("FactsEditor_SearchHintText", "Search..."))
				.ToolTipText(LOCTEXT("FactsEditor_TooltipText", "Search facts by tag. You can search by string ('Quest2.Trigger') or by several strings, separated by spaces ('Quest Trigger')\n"
													   "Press Enter to save this text as a toggle"))
				.OnTextChanged(this, &SFactsEditor::HandleSearchTextChanged)
				.OnTextCommitted( this, &SFactsEditor::HandleSearchTextCommitted )
			]

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(4.f, 1.f, 0.f, 1.f)
			[
				SAssignNew( OptionsButton, SComboButton )
				.ContentPadding(4.f)
				.ToolTipText(LOCTEXT("ShowOptions_Tooltip", "Show options to affect the visibility of items in the Facts Debugger"))
				.ComboButtonStyle( FAppStyle::Get(), "SimpleComboButtonWithIcon" ) // Use the tool bar item style for this button
				.OnGetMenuContent( this, &SFactsEditor::HandleGenerateOptionsMenu)
				.HasDownArrow(false)
				.ButtonContent()
				[
					SNew(SImage)
					.ColorAndOpacity(FSlateColor::UseForeground())
					.Image( FAppStyle::Get().GetBrush("Icons.Settings") )
				]
			]
		]
		
		// -------------------------------------------------------------------------------------------------------------
		// Search toggles

		+ SVerticalBox::Slot()
		.Padding( FMargin{ 8.f, 4.f, 8.f, 4.f } )
		.AutoHeight()
		[
			SAssignNew( SearchesHBox, SHorizontalBox )
			.Visibility_Lambda( [ this ]()
			{
				const bool bShouldBeVisible = SearchesContainer.IsValid() && SearchesContainer->GetChildren()->Num() > 0;
				return bShouldBeVisible ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
			} )

			// -------------------------------------------------------------------------------------------------------------
			// Search toggles

			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Fill )
			[
				SAssignNew( SearchesContainer, SWrapBox )
				.InnerSlotPadding( FVector2d{ 6, 4 } )
				.UseAllottedSize( true )
			]

			// -------------------------------------------------------------------------------------------------------------
			// Clear selected toggles button
			
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 8.f, 1.f, 2.f, 1.f )
			[
				SNew( SButton )
				.ToolTipText( LOCTEXT( "ClearSearchesButtonTooltip", "Clear all selected searches") )
				.ButtonStyle( &FAppStyle::Get(), "Button" )
				.ForegroundColor( FSlateColor::UseForeground() )
				.Visibility_Lambda( [ this ]()
				{
					const SFactsEditorSearchToggleRef* FoundElem = CurrentSearchToggles.FindByPredicate( []( const SFactsEditorSearchToggleRef& SearchToggle)
					{
						return SearchToggle->GetIsToggleChecked();
					} );

					return FoundElem ? EVisibility::Visible : EVisibility::Collapsed;
				} )
				.OnClicked( this, &SFactsEditor::HandleClearTogglesClicked)
				[
					SNew( STextBlock )
					.Text( LOCTEXT( "ClearSearchesButtonText", "Clear selected" ) )
					.Visibility( EVisibility::SelfHitTestInvisible )
					.ColorAndOpacity( FSlateColor::UseForeground() )
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
				.ToolTipText( LOCTEXT( "RemoveSearchesButtonTooltip", "Remove all searches from the facts editor") )
				.ButtonStyle( &FAppStyle::Get(), "Button" )
				.ForegroundColor( FSlateColor::UseForeground() )
				.OnClicked_Lambda( [ this ]()
				{
					SearchesContainer->ClearChildren();
					CurrentSearchToggles.Empty();
					FilterItems();

					return FReply::Handled();
				} )
				[
					SNew( SImage )
					.Visibility( EVisibility::SelfHitTestInvisible )
					.Image( FAppStyle::Get().GetBrush( "Icons.X" ) )
					.ColorAndOpacity( FSlateColor::UseForeground() )
				]
			]
		]
		

		// -------------------------------------------------------------------------------------------------------------
		// FactsTree

		+ SVerticalBox::Slot()
		.FillHeight( 1.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
			[
				SAssignNew( Splitter, SSplitter )
				.Orientation( GetDefault< UFactsDebuggerSettingsLocal >()->Orientation )

				+ SSplitter::Slot()
				[
					SAssignNew( FavoriteFactsTreeView, SFactsTreeView )
					.TreeItemsSource( &FavoritesRootItem->Children )
					.OnGenerateRow( this, &SFactsEditor::OnGenerateWidgetForFactsTreeView )
					.OnGetChildren( this, &SFactsEditor::OnGetChildren )
					.OnExpansionChanged( this, &SFactsEditor::HandleItemExpansionChanged )
					.OnGeneratePinnedRow( this, &SFactsEditor::HandleGeneratePinnedTreeRow )
					.ShouldStackHierarchyHeaders_Lambda( []() { return GetDefault< UFactsDebuggerSettingsLocal >()->bShouldStackHierarchyHeaders; } )
					.HeaderRow
					(
						SNew( SHeaderRow )

						+ SHeaderRow::Column( "Favorites" )
						.FixedWidth( 24.f )
						.HAlignHeader(HAlign_Center)
						.VAlignHeader(VAlign_Center)
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
						// .DefaultTooltip( LOCTEXT( "Favorites_ToolTip", "Removes Fact from Favorites Tree" ) )
						[
							SNew(SImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
							.Image( FAppStyle::Get().GetBrush("Icons.Star") )
						]
						
						+ SHeaderRow::Column( "FactTag" )
						.SortPriority( EColumnSortPriority::Secondary )
						.DefaultLabel( LOCTEXT( "FactTag", "Tag" ) )

						+ SHeaderRow::Column( "FactValue" )
						.ManualWidth( 100.f )
						.DefaultLabel( LOCTEXT( "FactValue", "Value" ) )
						.DefaultTooltip( LOCTEXT( "FactValue_Tooltip", "Current value of this fact. Undefined means that value for this fact was not yet set" ) )
					)
				]

				+ SSplitter::Slot()
				[
					SAssignNew( FactsTreeView, SFactsTreeView )
					.TreeItemsSource( &FilteredRootItem->Children )
					.OnItemToString_Debug( this, &SFactsEditor::OnItemToStringDebug )
					.OnGenerateRow( this, &SFactsEditor::OnGenerateWidgetForFactsTreeView )
					.OnGetChildren( this, &SFactsEditor::OnGetChildren)
					.OnExpansionChanged( this, &SFactsEditor::HandleItemExpansionChanged )
					.OnGeneratePinnedRow( this, &SFactsEditor::HandleGeneratePinnedTreeRow )
					.ShouldStackHierarchyHeaders_Lambda( []() { return GetDefault< UFactsDebuggerSettingsLocal >()->bShouldStackHierarchyHeaders; } )
					.HeaderRow
					(
						SNew( SHeaderRow )

						+ SHeaderRow::Column( "Favorites" )
						.FixedWidth( 24.f )
						.HAlignHeader(HAlign_Center)
						.VAlignHeader(VAlign_Center)
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
						// .DefaultTooltip( LOCTEXT( "Favorites_ToolTip", "Moves Fact to Favorites Tree" ) )
						[
							SNew(SImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
							.Image( FAppStyle::Get().GetBrush("Icons.Star") )
						]
						
						+ SHeaderRow::Column( "FactTag" )
						.SortPriority( EColumnSortPriority::Secondary )
						.DefaultLabel( LOCTEXT( "FactTag", "Tag" ) )

						+ SHeaderRow::Column( "FactValue" )
						.ManualWidth( 100.f )
						.DefaultLabel( LOCTEXT( "FactValue", "Value" ) )
						.DefaultTooltip( LOCTEXT( "FactValue_Tooltip", "Current value of this fact. Undefined means that value for this fact was not yet set" ) )
					)
				]
			]
		]
	];
	
	FSimpleFactsEditorModule::Get().OnGameInstanceStarted.BindRaw( this, &SFactsEditor::HandleGameInstanceStarted );
	if ( InArgs._bIsGameStarted )
	{
		HandleGameInstanceStarted();
	}

	LoadSettings();
	RestoreExpansionState();
	FilterItems();
}

TArray<FSearchToggleState> SFactsEditor::GetSearchToggleStates()
{
	TArray< FSearchToggleState > SearchToggleStates;
	for ( const SFactsEditorSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		SearchToggleStates.Emplace( SearchToggle->GetIsToggleChecked(), SearchToggle->GetSearchText() );
	}

	return SearchToggleStates;
}

void SFactsEditor::LoadFactsPreset( UFactsPreset* InPreset )
{
	LoadedPreset = InPreset;
	LoadFactsPresetRecursive( InPreset, RootItem );
}

void SFactsEditor::LoadFactsPresetRecursive( UFactsPreset* InPreset, const FFactTreeItemPtr& FactItem ) const
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

void SFactsEditor::HandleGameInstanceStarted()
{
	InitItem( FilteredRootItem.ToSharedRef() );
	InitItem( FavoritesRootItem.ToSharedRef() );
}

void SFactsEditor::InitItem( FFactTreeItemRef Item )
{
	Item->InitPIE();

	for ( FFactTreeItemPtr& ChildItem : Item->Children )
	{
		InitItem( ChildItem->AsShared() );
	}
}

TSharedRef<ITableRow> SFactsEditor::OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr InItem,
                                                                      const TSharedRef<STableViewBase>& TableViewBase )
{
	class SFactTreeItem : public SMultiColumnTableRow< FFactTreeItemPtr >
	{
	public:
		SLATE_BEGIN_ARGS( SFactTreeItem ) {}
		SLATE_END_ARGS()

		void Construct( const FArguments& InArgs, const TSharedRef< STableViewBase > InOwnerTable, SFactsEditor* InFactsEditor, FFactTreeItemPtr InItem )
		{
			Item = InItem;
			bShowFullName = GetDefault< UFactsDebuggerSettingsLocal >()->bShowFullFactNames;
			FactsEditor = InFactsEditor;
			
			SMultiColumnTableRow::Construct( FSuperRowType::FArguments()
				.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>( "ContentBrowser.AssetListView.ColumnListTableRow" ) ), InOwnerTable );

			FavoriteBrush = FAppStyle::Get().GetBrush( "Icons.Star" );
			NormalBrush = FFactsEditorStyleStyle::Get().GetBrush( "Icons.Star.Outline" );
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
			else if ( InColumnName == "FactTag" )
			{
				return SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SExpanderArrow, SharedThis( this ) )
						.ShouldDrawWires( true )
					]
					+SHorizontalBox::Slot()
					.FillWidth( 1.f )
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.ColorAndOpacity( Item->Tag.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground() )
						.Text( FText::FromString( bShowFullName ? Item->Tag.ToString() : Item->TagNode->GetSimpleTagName().ToString() ) )
						.HighlightText_Lambda( [ this ](){ return FactsEditor->CurrentSearchText; } )
					];
			}
			else if ( InColumnName == "FactValue" )
			{
				return SNew( SNumericEntryBox< int32 > )
					.Value_Raw( Item.Get(), &FFactTreeItem::GetValue )
					.OnValueCommitted( FOnInt32ValueCommitted::CreateRaw( Item.Get(), &FFactTreeItem::HandleNewValueCommited ) )
					.UndeterminedString( LOCTEXT( "FactUndefinedValue", "undefined") );
			}
			else
			{
				return SNew( STextBlock ).Text( LOCTEXT( "UnknownColumn", "Unknown Column" ) );
			}
		}
		
		FReply HandleFavoriteClicked()
		{
			if ( SFactsEditor::FavoriteFacts.Contains( Item->Tag ) )
			{
				SFactsEditor::FavoriteFacts.Remove( Item->Tag );
			}
			else
			{
				SFactsEditor::FavoriteFacts.Add( Item->Tag );
			}
			
			FactsEditor->SaveSettings();
			FactsEditor->FilterItems();
			return FReply::Handled();
		}

		const FSlateBrush* GetItemBrush() const
		{
			return IsFavorite() ? FavoriteBrush : NormalBrush;
		}

		FSlateColor GetItemColor() const
		{
			const bool bIsSelected = FactsEditor->FactsTreeView->IsItemSelected( Item.ToSharedRef() );
		
			if ( IsFavorite() == false )
			{
				if (IsHovered() == false && bIsSelected == false)
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
			
			return Algo::AnyOf( SFactsEditor::FavoriteFacts, MatchFavorites );
		}

	private:

		FFactTreeItemPtr Item;
		bool bShowFullName = false;
		SFactsEditor* FactsEditor;

		const FSlateBrush* FavoriteBrush = nullptr;
		const FSlateBrush* NormalBrush = nullptr;
	};

	if ( InItem.IsValid() )
	{
		return SNew( SFactTreeItem, TableViewBase, this, InItem );
	}
	else
	{
		return SNew(STableRow< TSharedPtr<FString> >, TableViewBase)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UnknownItemType", "Unknown Item Type"))
			];		
	}
}

TSharedRef< ITableRow > SFactsEditor::HandleGeneratePinnedTreeRow( FFactTreeItemPtr FactTreeItem,
	const TSharedRef<STableViewBase>& TableViewBase )
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

void SFactsEditor::OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray<FFactTreeItemPtr>& Children )
{
	if ( FactTreeItem.IsValid() )
	{
		Children.Append( FactTreeItem->Children );
	}
}

void SFactsEditor::HandleItemExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded )
{
	if ( bIsRestoringExpansion || RootItem != FilteredRootItem )
	{
		return;
	}
	
	if ( bInExpanded )
	{
		CollapsedStates.Remove( FactTreeItem->Tag );
	}
	else
	{
		CollapsedStates.Add( FactTreeItem->Tag );
	}
}

TSharedRef<SWidget> SFactsEditor::HandleGeneratePresetsMenu()
{
	FMenuBuilder MenuBuilder{true, nullptr};

	FUIAction PresetNameAction = FUIAction();
	PresetNameAction.CanExecuteAction = FCanExecuteAction::CreateLambda( []() { return false; } );

	MenuBuilder.AddMenuEntry(
		LOCTEXT( "CurrentPreset_Text", "Current preset:"),
		LOCTEXT( "CurrentPreset_Tooltip", "Current"),
		FSlateIcon(),
		PresetNameAction,
		NAME_None,
		EUserInterfaceActionType::None
	);

	MenuBuilder.AddSeparator(  );

	MenuBuilder.AddMenuEntry(
		LOCTEXT( "SavePreset_Text", "Save preset" ),
		LOCTEXT( "SavePreset_Tooltip", "Save the current preset" ),
		FSlateIcon(FAppStyle::Get().GetStyleSetName(), "AssetEditor.SaveAsset" ),
		FUIAction(FExecuteAction::CreateLambda( [ this ]() { } ) )
	);

	MenuBuilder.BeginSection( NAME_None, LOCTEXT( "LoadPreset_MenuSection", "Load preset" ));
	{
		TArray<FAssetData> AssetData;
		// UAssetManager::Get().GetPrimaryAssetDataList( FPrimaryAssetType(UFactsPreset::StaticClass()->GetFName()), AssetData );
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

TSharedRef< SWidget > SFactsEditor::HandleGenerateOptionsMenu()
{
	FMenuBuilder MenuBuilder( false, nullptr );

	MenuBuilder.BeginSection( "", LOCTEXT( "Options_HierarchySectionName", "Hierarchy" ) );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ExpandAll", "Expand All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsEditor::HandleExpandAllClicked ) )
		);
	
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_CollapseAll", "Collapse All" ),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction( FExecuteAction::CreateRaw( this, &SFactsEditor::HandleCollapseAllClicked ) )
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("Options_ShowHierarchy", "Stack Hierarchy Headers"),
			LOCTEXT("Options_ShowHierarchy_Tooltip", "Toggle pinning of the hierarchy of items at the top of the outliner"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw( this, &SFactsEditor::HandleShouldStackHierarchyHeadersClicked ),
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
			LOCTEXT( "Options_ShowRootTag_Tooltip", "Show Root Fact Tag" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &SFactsEditor::HandleShowRootTagClicked ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bShowRootFactTag; })
				),
			NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_ShowFullNames", "Show Full Fact Names" ),
			LOCTEXT( "Options_ShowFullNames_Tooltip", "Show Full Fact Names" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &SFactsEditor::HandleShowFullNamesClicked ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bShowFullFactNames; })
				),
		   NAME_None,
		   EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT( "Options_RemoveFavorites", "Remove Favorites from Main Tree" ),
			LOCTEXT( "Options_RemoveFavorites_Tooltip", "Remove Favorites from Main Tree" ),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &SFactsEditor::HandleRemoveFavoritesClicked ),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda( [](){ return GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree; })
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
			FExecuteAction::CreateRaw( this, &SFactsEditor::HandleOrientationChanged, Orient_Horizontal ),
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
			FExecuteAction::CreateRaw( this, &SFactsEditor::HandleOrientationChanged, Orient_Vertical ),
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

void SFactsEditor::HandleSearchTextChanged( const FText& SearchText )
{
	CurrentSearchText = SearchText;
	FilterItems();
}

void SFactsEditor::HandleSearchTextCommitted( const FText& SearchText, ETextCommit::Type Type )
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

	for ( const SFactsEditorSearchToggleRef& SearchToggle : CurrentSearchToggles )
	{
		const FText ToggleText = SearchToggle->GetSearchText();
		ExistingStrings.Add( ToggleText.ToString() );

		if (SearchText.EqualToCaseIgnored( ToggleText ) )
		{
			SearchToggle->SetIsButtonChecked( true );
		}
	}

	if ( ExistingStrings.Contains( SearchText.ToString() ))
	{
		return;
	}

	SFactsEditorSearchToggleRef NewSearchToggle =
		SNew( SFactsEditorSearchToggle, SearchText )
		.OnClickedOnce( this, &SFactsEditor::HandleSearchToggleClicked )
		.OnRightButtonClicked( this, &SFactsEditor::HandleRemoveSearchToggle )
		.OnAltClicked( this, &SFactsEditor::HandleRemoveSearchToggle );

	CurrentSearchToggles.Add( NewSearchToggle );

	RefreshSearchToggles();
	SearchBox->SetText( FText::GetEmpty() );

	SaveSettings();
}

void SFactsEditor::FilterItems()
{
	FilteredRootItem.Reset();
	FavoritesRootItem.Reset();

	// Parse filter strings
	TArray< FString > ActiveTogglesText;
	for ( const SFactsEditorSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		if ( SearchToggle->GetIsToggleChecked() )
		{
			ActiveTogglesText.Add( SearchToggle->GetSearchText().ToString() );
		}
	}

	TArray< FString > Tokens;
	CurrentSearchText.ToString().ParseIntoArray( Tokens, TEXT( "&" ) );

	// Reset containers
	FilteredRootItem = MakeShared< FFactTreeItem >();
	FavoritesRootItem = MakeShared< FFactTreeItem >();

	// Filtering
	Utils::FFilterOptions Options{ ActiveTogglesText, Tokens, false };
	Utils::FilterFactItemChildren( RootItem->Children,  FilteredRootItem->Children, Options );

	Utils::FFilterOptions FavoritesOptions{ ActiveTogglesText, Tokens, true };
	Utils::FilterFactItemChildren( RootItem->Children,  FavoritesRootItem->Children, FavoritesOptions );
	
	FactsTreeView->SetTreeItemsSource( &FilteredRootItem->Children );
	FavoriteFactsTreeView->SetTreeItemsSource( &FavoritesRootItem->Children );
	
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true );

	FactsTreeView->RequestTreeRefresh();
	FavoriteFactsTreeView->RequestTreeRefresh();
}

void SFactsEditor::FilterFactItemChildren(TArray<FFactTreeItemPtr>& SourceArray, TArray<FFactTreeItemPtr>& OutDestArray,
	const FFilterOptions& Options)
{
	for (const FFactTreeItemPtr& SourceItem : SourceArray)
	{
		// lambda for filtering by search toggles
		auto MatchSearchToggles = [Tag = SourceItem->Tag.ToString()]( const FString& SearchText )
		{
			TArray<FString> Tokens;
			SearchText.ParseIntoArray( Tokens, TEXT("&") );

			
			return Algo::AllOf( Tokens, [Tag]( const FString& Token )
			{
				return Tag.Contains( Token );
			} );
		};
		// lambda for filtering by search box
		auto MatchSearchBox = [Tag = SourceItem->Tag.ToString()]( const FString& Token )
		{
			return Tag.Contains( Token );
		};

		enum class ETagCheckResult
		{
			None,
			Partial,
			Full
		};
		
		// lambda for checking favorite
		auto CheckFavorites = [Tag = SourceItem->Tag]()
		{
			bool bPartialMatch = false;
			for ( FFactTag FavoriteFact : SFactsEditor::FavoriteFacts )
			{
				if ( Tag == FavoriteFact )
				{
					return ETagCheckResult::Full;
				}
				if ( FavoriteFact.MatchesTag( Tag ) )
				{
					bPartialMatch = true;
				}
			}

			if ( bPartialMatch )
			{
				return ETagCheckResult::Partial;
			}

			return ETagCheckResult::None;
		};

		const UFactsDebuggerSettingsLocal* Settings = GetDefault< UFactsDebuggerSettingsLocal >();

		ETagCheckResult Result = CheckFavorites();

		bool bMatched = ( Options.SearchToggleStrings.IsEmpty() || Algo::AnyOf( Options.SearchToggleStrings, MatchSearchToggles ) ) && Algo::AllOf( Options.SearchBarStrings, MatchSearchBox );

		// todo: refactor
		switch (Result) {
		case ETagCheckResult::None: // Fact and it's parent tags is not in favorites
			{
				if ( Options.bFilterFavorites )
				{
					// skip
				}
				else
				{
					if ( bMatched )
					{
						FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
						*NewItem = *SourceItem;
						NewItem->InitItem();
					}
					else
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
				}
			}
			break;
		case ETagCheckResult::Partial: // Fact is not in favorites, but one of the parent tags is 
			{
				// find actual favorite children
				if ( Options.bFilterFavorites ) 
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
					if ( GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree == false )
					{
						if ( bMatched )
						{
							FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
							*NewItem = *SourceItem;
							NewItem->InitItem();
						}
						else
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
					}
					else
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
				}
			}
			break;
		case ETagCheckResult::Full: // Fact is favorite
			{
				if ( Options.bFilterFavorites )
				{
					if ( bMatched )
					{
						FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
						*NewItem = *SourceItem;
						NewItem->InitItem();
					}
					else
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
				}
				else
				{
					if ( GetDefault< UFactsDebuggerSettingsLocal >()->bRemoveFavoritesFromMainTree == false )
					{
						if ( bMatched )
						{
							FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
							*NewItem = *SourceItem;
							NewItem->InitItem();
						}
						else
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
					}
				}
			}
			break;
		}
	}
}

void SFactsEditor::HandleExpandAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, true );
	OptionsButton->SetIsOpen( false );
}

void SFactsEditor::HandleCollapseAllClicked()
{
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, false );
	SetItemsExpansion( FavoriteFactsTreeView, FavoritesRootItem->Children, false );
	OptionsButton->SetIsOpen( false );
}

void SFactsEditor::SetItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, TArray<FFactTreeItemPtr> FactItems, bool bShouldExpand )
{
	for ( const FFactTreeItemPtr& Item : FactItems )
	{
		TreeView->SetItemExpansion( Item, bShouldExpand );
		SetItemsExpansion( TreeView, Item->Children, bShouldExpand );
	}
}

void SFactsEditor::RestoreExpansionState()
{
	TGuardValue< bool > RestoringExpansion( bIsRestoringExpansion, true );
	
	// Default state is expanded.
	SetItemsExpansion( FactsTreeView, FilteredRootItem->Children, true );
	SetItemsExpansion( FactsTreeView, FavoritesRootItem->Children, true );

	for ( const FFactTag& FactTag : CollapsedStates )
	{
		TArray< FFactTreeItemPtr > Path;
		if ( FindItemByTagRecursive( FilteredRootItem, FactTag, Path) )
		{
			FactsTreeView->SetItemExpansion(Path.Last(), false);
		}
		else if ( FindItemByTagRecursive( FavoritesRootItem, FactTag, Path ) )
		{
			FavoriteFactsTreeView->SetItemExpansion(Path.Last(), false);
		}
	}
}

bool SFactsEditor::FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag,
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

FReply SFactsEditor::HandleRemoveSearchToggle()
{
	CleanupSearchesMarkedForDelete();
	RefreshSearchToggles();
	FilterItems();
	SaveSettings();

	return FReply::Handled();
}

void SFactsEditor::CleanupSearchesMarkedForDelete()
{
	CurrentSearchToggles.RemoveAllSwap( [](const SFactsEditorSearchToggleRef& SearchToggle )
	{
		return SearchToggle->GetIsMarkedForDelete();
	} );
}

void SFactsEditor::RefreshSearchToggles()
{
	SearchesContainer->ClearChildren();
	
	for ( const SFactsEditorSearchToggleRef& SearchToggle: CurrentSearchToggles )
	{
		SearchesContainer->AddSlot()
		[
			SearchToggle
		];
	}
}

void SFactsEditor::CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates )
{
	for ( FSearchToggleState& ToggleState : SearchToggleStates )
	{
		SFactsEditorSearchToggleRef NewSearchToggle =
			SNew( SFactsEditorSearchToggle, ToggleState.SearchText )
			.OnClickedOnce( this, &SFactsEditor::HandleSearchToggleClicked )
			.OnRightButtonClicked( this, &SFactsEditor::HandleRemoveSearchToggle )
			.OnAltClicked( this, &SFactsEditor::HandleRemoveSearchToggle );

		NewSearchToggle->SetIsButtonChecked( ToggleState.bIsToggleChecked );

		CurrentSearchToggles.Add( NewSearchToggle );
	}

	RefreshSearchToggles();
}

FReply SFactsEditor::HandleClearTogglesClicked()
{
	for ( const SFactsEditorSearchToggleRef& SearchToggle : CurrentSearchToggles )
	{
		SearchToggle->SetIsButtonChecked( false );
	}

	FilterItems();
	SaveSettings();

	return FReply::Handled();
}

FReply SFactsEditor::HandleSearchToggleClicked()
{
	FilterItems();
	SaveSettings();
	return FReply::Handled();
}

void SFactsEditor::BuildFactTreeItems()
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
	
	FilteredRootItem = RootItem;
	FavoritesRootItem = MakeShared< FFactTreeItem >();
}

FFactTreeItemPtr SFactsEditor::BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode )
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

FString SFactsEditor::OnItemToStringDebug( FFactTreeItemPtr FactTreeItem ) const
{
	FStringFormatOrderedArguments Args;
	Args.Add( FactTreeItem->Tag.ToString() );
	Args.Add( FactTreeItem->Value.IsSet() ? FString::FromInt( FactTreeItem->Value.GetValue() ) : "Undefined" );
	return FString::Format( TEXT("{0} {1]"), Args );
}

void SFactsEditor::LoadSettings()
{
	UFactsDebuggerSettingsLocal* FactsDebuggerSettings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	FactsDebuggerSettings->OnChangedDelegate.BindSP( this , &SFactsEditor::HandleSettingsChanged );

	CreateDefaultSearchToggles( FactsDebuggerSettings->ToggleStates );
	SFactsEditor::FavoriteFacts = FactsDebuggerSettings->FavoriteFacts;
	
}

void SFactsEditor::SaveSettings()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	// just copy now, it shouldn't be much elements in array. It it will be a problem - optimize
	Settings->ToggleStates = GetSearchToggleStates();
	Settings->FavoriteFacts = SFactsEditor::FavoriteFacts;
	Settings->SaveConfig();
}

void SFactsEditor::HandleSettingsChanged()
{
	// just rebuild the tree
	BuildFactTreeItems();
	FilterItems();
}

// todo: refactor it somehow, to avoid duplicating the same code for changing just one property
void SFactsEditor::HandleShowRootTagClicked()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->bShowRootFactTag = !Settings->bShowRootFactTag;
	Settings->SaveConfig();
	
	HandleSettingsChanged();
}

void SFactsEditor::HandleShowFullNamesClicked()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->bShowFullFactNames = !Settings->bShowFullFactNames;
	Settings->SaveConfig();
	
	HandleSettingsChanged();
}

void SFactsEditor::HandleRemoveFavoritesClicked()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->bRemoveFavoritesFromMainTree = !Settings->bRemoveFavoritesFromMainTree;
	Settings->SaveConfig();
	
	HandleSettingsChanged();
}

void SFactsEditor::HandleShouldStackHierarchyHeadersClicked()
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->bShouldStackHierarchyHeaders = !Settings->bShouldStackHierarchyHeaders;
	Settings->SaveConfig();
	
	HandleSettingsChanged();
}

void SFactsEditor::HandleOrientationChanged( EOrientation Orientation )
{
	UFactsDebuggerSettingsLocal* Settings = GetMutableDefault< UFactsDebuggerSettingsLocal >();
	Settings->Orientation = Orientation;
	Settings->SaveConfig();
	
	Splitter->SetOrientation( Orientation );
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


#undef LOCTEXT_NAMESPACE