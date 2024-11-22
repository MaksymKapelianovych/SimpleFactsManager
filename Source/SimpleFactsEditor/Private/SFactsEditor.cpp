// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditor.h"

#include "ContentBrowserModule.h"
#include "FactsPreset.h"
#include "FactSubsystem.h"
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

FFactTreeItem::~FFactTreeItem()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsEditorModule::Get().TryGetFactSubsystem() )
	{
		FactSubsystem->GetOnFactValueChangedDelegate( Tag ).Remove( Handle );
	}
}

void FFactTreeItem::InitPIE()
{
	if ( UFactSubsystem* FactSubsystem = FSimpleFactsEditorModule::Get().TryGetFactSubsystem() )
	{
		Handle = FactSubsystem->GetOnFactValueChangedDelegate( Tag ).AddSP( AsShared(), &FFactTreeItem::HandleValueChanged );
	}
}

void FFactTreeItem::HandleValueChanged( int32 NewValue )
{
	Value = NewValue;
}

void FFactTreeItem::HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type ) const
{
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
			SNew(SComboButton)
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
			SNew(SSearchBox)
			.HintText(LOCTEXT("FactsEditor_SearchHintText", "Search..."))
			.ToolTipText(LOCTEXT("FactsEditor_TooltipText", "Search facts by tag. You can search by string ('Quest2.Trigger') or by several strings, separated by spaces ('Quest Trigger')\n"
												   "Press Enter to save this text as a toggle"))
			.OnTextChanged(this, &SFactsEditor::HandleSearchTextChanged)
			.OnTextCommitted( this, &SFactsEditor::HandleSearchTextCommitted )
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
				SAssignNew( FactsTreeView, SFactsTreeView )
				.TreeItemsSource( &FilteredRootItem->Children )
				.OnItemToString_Debug( this, &SFactsEditor::OnItemToStringDebug )
				.OnGenerateRow( this, &SFactsEditor::OnGenerateWidgetForFactsTreeView )
				.OnGetChildren( this, &SFactsEditor::OnGetChildren)
				.OnExpansionChanged( this, &SFactsEditor::HandleItemExpansionChanged )
				.OnGeneratePinnedRow( this, &SFactsEditor::HandleGeneratePinnedTreeRow )
				.ShouldStackHierarchyHeaders( true )
				.HeaderRow
				(
					SNew( SHeaderRow )
					
					+ SHeaderRow::Column( "FactTag" )
					.DefaultLabel( LOCTEXT( "FactTag", "Tag" ) )

					+ SHeaderRow::Column( "FactValue" )
					.DefaultLabel( LOCTEXT( "FactValue", "Value" ) )
					.ManualWidth( 100.f )
				)
			]
		]
	];
	FSimpleFactsEditorModule::Get().OnGameInstanceStarted.BindRaw( this, &SFactsEditor::HandleGameInstanceStarted );


	CreateDefaultSearchToggles( {} ); // todo: change
	RestoreExpansionState();
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
	// todo: temporary commented out, fix
	// for ( FFactTreeItemPtr& FactTreeItem : AllFactTreeItems )
	// {
	// 	if ( int32* PresetValue = InPreset->PresetValues.Find( FactTreeItem->Tag ) )
	// 	{
	// 		FactTreeItem->HandleNewValueCommited( *PresetValue, ETextCommit::Type::Default );
	// 		FactTreeItem->HandleValueChanged( *PresetValue ); // todo handle if load is in editor, but not PIE
	// 	}
	// }
}

void SFactsEditor::HandleGameInstanceStarted()
{
	InitItem( RootItem.ToSharedRef() );
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

		void Construct( const FArguments& InArgs, const TSharedRef< SFactsTreeView > InOwnerTable, FFactTreeItemPtr InItem )
		{
			Item = InItem;
			SMultiColumnTableRow::Construct( FSuperRowType::FArguments()
				.Style( &FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.AlternatingRow") ), InOwnerTable );
			
			// SetBorderBackgroundColor( FSlateColor{FLinearColor::Green} );
		}
		
		virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& InColumnName ) override
		{
			if ( InColumnName == "FactTag" )
			{
				return SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew( SExpanderArrow, SharedThis( this ) )
					]
					+SHorizontalBox::Slot()
					.FillWidth( 1.f )
					.VAlign( VAlign_Center )
					[
						SNew( STextBlock )
						.ColorAndOpacity( Item->Tag.IsValid() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground() )
						.Text( FText::FromString( Item->TagNode->GetSimpleTagName().ToString() ) )
					];
			}
			else if ( InColumnName == "FactValue" )
			{
				return SNew( SNumericEntryBox< int32 > )
					.Value_Lambda( [this](){ return Item->Value; } )
					.OnValueCommitted( FOnInt32ValueCommitted::CreateRaw( Item.Get(), &FFactTreeItem::HandleNewValueCommited ) )
					.UndeterminedString( LOCTEXT( "FactUndefinedValue", "Undefined") );
			}
			else
			{
				return SNew( STextBlock ).Text( LOCTEXT( "UnknownColumn", "Unknown Column" ) );
			}
		}

	private:

		FFactTreeItemPtr Item;
	};

	if ( InItem.IsValid() )
	{
		return SNew( SFactTreeItem, FactsTreeView.ToSharedRef(), InItem );
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
	return SNew( STableRow< TSharedPtr< FString > >, TableViewBase )
		[
			SNew( SBox )
			.HeightOverride( 22.f )
			.VAlign( VAlign_Center )
			[
				SNew( STextBlock )
				.Text( FText::FromString( FactTreeItem->TagNode->GetSimpleTagName().ToString() ) )
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

TSharedRef<SWidget> SFactsEditor::HandleGeneratePresetsMenu() const
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


		TSharedRef< SWidget > PresetPicker = SNew( SBox )
			.WidthOverride( 300.f )
			.HeightOverride( 300.f )
			[
				SNew( SFactsPresetPicker, AssetData )
			];

		MenuBuilder.AddWidget( PresetPicker, FText(), true, false );
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget(  );
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
}

void SFactsEditor::FilterItems()
{
	FilteredRootItem.Reset();
	FFactTreeItemPtr TempVisibleItems;
	bool bExpandItems = false;

	// first pass - find all items, that should be visible by search toggles
	TArray< FString > ActiveTogglesText;
	
	for (const SFactsEditorSearchToggleRef SearchToggle : CurrentSearchToggles )
	{
		if ( SearchToggle->GetIsToggleChecked() )
		{
			ActiveTogglesText.Add( SearchToggle->GetSearchText().ToString() );
		}
	}
		
	if ( ActiveTogglesText.Num() > 0 )
	{
		TempVisibleItems = MakeShared< FFactTreeItem >();
		FilterFactItemChildren( ActiveTogglesText, EFactFilterMode::SearchToggles, RootItem->Children, TempVisibleItems->Children);
		bExpandItems = true;
	}
	else
	{
		TempVisibleItems = RootItem;
	}

	// second pass - find all visible item among previously filtered by search text
	if ( CurrentSearchText.IsEmpty() )
	{
		FilteredRootItem = TempVisibleItems;
		if ( bExpandItems )
		{
			ExpandAll( FilteredRootItem->Children );
		}
		else
		{
			RestoreExpansionState();
		}
		FactsTreeView->SetTreeItemsSource( &FilteredRootItem->Children );
		FactsTreeView->RequestTreeRefresh();
		return;
	}

	TArray< FString > Tokens;
	CurrentSearchText.ToString().ParseIntoArray( Tokens, TEXT( "&" ) );
	FilteredRootItem = MakeShared< FFactTreeItem >();
	FilterFactItemChildren( Tokens, EFactFilterMode::SearchBox, TempVisibleItems->Children, FilteredRootItem->Children );
	FactsTreeView->SetTreeItemsSource( &FilteredRootItem->Children );
	ExpandAll( FilteredRootItem->Children );

	FactsTreeView->RequestTreeRefresh();
}

void SFactsEditor::FilterFactItemChildren( TArray<FString> FilterStrings, EFactFilterMode FilterMode,
	TArray<FFactTreeItemPtr>& SourceArray, TArray<FFactTreeItemPtr>& OutDestArray )
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

		bool bMatched = false;

		switch ( FilterMode ) {
		case SearchBox:
			bMatched = Algo::AllOf( FilterStrings, MatchSearchBox );
			break;
		case SearchToggles:
			bMatched = Algo::AnyOf( FilterStrings, MatchSearchToggles );
			break;
		}

		if ( bMatched )
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
		}
		else
		{
			TArray< FFactTreeItemPtr > FilteredChildren;
			FilterFactItemChildren( FilterStrings, FilterMode, SourceItem->Children, FilteredChildren );
			if ( FilteredChildren.Num() )
			{
				FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
				*NewItem = *SourceItem;
				NewItem->Children = FilteredChildren;
			}
		}
	}
}

void SFactsEditor::ExpandAll(TArray<FFactTreeItemPtr> FactItems)
{
	for ( const FFactTreeItemPtr& Item : FactItems )
	{
		FactsTreeView->SetItemExpansion( Item, true );
		ExpandAll( Item->Children );
	}
}

void SFactsEditor::RestoreExpansionState()
{
	TGuardValue< bool > RestoringExpansion( bIsRestoringExpansion, true );
	
	// Default state is expanded.
	ExpandAll( FilteredRootItem->Children );

	for ( const FFactTag& FactTag : CollapsedStates )
	{
		// for ( const FFactTreeItemPtr& Item : FilteredFactTreeItems )
		// {
			TArray< FFactTreeItemPtr > Path;
			if ( FindItemByTagRecursive( FilteredRootItem, FactTag, Path))
			{
				FactsTreeView->SetItemExpansion(Path.Last(), false);
			}
		// }
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

	return FReply::Handled();
}

FReply SFactsEditor::HandleSearchToggleClicked()
{
	FilterItems();
	return FReply::Handled();
}

void SFactsEditor::BuildFactTreeItems()
{
	RootItem = MakeShared< FFactTreeItem >();

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	TSharedPtr< FGameplayTagNode > Node = Manager.FindTagNode( FFactTag::GetRootTag() );
	BuildFactItem( RootItem, Node );
	FilteredRootItem = RootItem;
	// Default state is expanded
	// ExpandAll( FilteredRootItem->Children );

	// CurrentSearchText = FText::GetEmpty();
	// FilterItems();
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

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


#undef LOCTEXT_NAMESPACE