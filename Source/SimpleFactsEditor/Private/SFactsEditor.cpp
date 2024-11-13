// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditor.h"

#include "ContentBrowserModule.h"
#include "FactsPreset.h"
#include "FactSubsystem.h"
#include "IContentBrowserSingleton.h"
#include "SFactsEditorSearchToggle.h"
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

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

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

void SFactsEditor::Construct( const FArguments& InArgs )
{
	FSimpleFactsEditorModule::Get().OnGameInstanceStarted.BindRaw( this, &SFactsEditor::HandleGameInstanceStarted );
	
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
			// .HAlign( HAlign_Right )
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
					// HandleSearchTextChanged( FText::GetEmpty() );
					ExecuteSearch( CurrentSearchText );

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
				.TreeItemsSource( &AllFactTreeItems )
				.OnItemToString_Debug( this, &SFactsEditor::OnItemToStringDebug )
				.OnGenerateRow( this, &SFactsEditor::OnGenerateWidgetForFactsTreeView )
				.OnGetChildren( this, &SFactsEditor::OnGetChildren)
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

	CreateDefaultSearchToggles( {} ); // todo: change
	BuildFactTreeItems();
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
	for ( FFactTreeItemPtr& FactTreeItem : AllFactTreeItems )
	{
		if ( int32* PresetValue = InPreset->PresetValues.Find( FactTreeItem->Tag ) )
		{
			FactTreeItem->HandleNewValueCommited( *PresetValue, ETextCommit::Type::Default );
			FactTreeItem->HandleValueChanged( *PresetValue ); // todo handle if load is in editor, but not PIE
		}
	}
}

void SFactsEditor::HandleGameInstanceStarted()
{
	for ( FFactTreeItemPtr& FactTreeItem : AllFactTreeItems )
	{
		InitItem( FactTreeItem.ToSharedRef() );
	}
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

void SFactsEditor::OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray<FFactTreeItemPtr>& Children )
{
	if ( FactTreeItem.IsValid() )
	{
		Children.Append( FactTreeItem->Children );
	}
}

TSharedRef<SWidget> SFactsEditor::HandleGeneratePresetsMenu() const
{
	FMenuBuilder MenuBuilder{true, nullptr};

	IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>( "ContentBrowser" ).Get();

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

	FAssetPickerConfig AssetPickerConfig;
	{
		AssetPickerConfig.SelectionMode = ESelectionMode::Single;
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
		AssetPickerConfig.bFocusSearchBoxWhenOpened = true;
		AssetPickerConfig.bAllowNullSelection = false;
		AssetPickerConfig.bShowPathInColumnView = true;

		AssetPickerConfig.AssetShowWarningText = LOCTEXT("NoPresets_Warning", "No Presets Found");
		AssetPickerConfig.Filter.ClassPaths.Add(UFactsPreset::StaticClass()->GetClassPathName());
		AssetPickerConfig.Filter.bRecursiveClasses = true;
		AssetPickerConfig.OnAssetSelected =
			FOnAssetSelected::CreateLambda( [ this ] (const FAssetData& InPresetData) {} );
	}

	MenuBuilder.BeginSection( NAME_None, LOCTEXT( "LoadPreset_MenuSection", "Load preset" ));
	{
		// if we are in editor and not playing - show this widget
		// else show custom combo box
		// TSharedRef<SWidget> PresetPicker = SNew(SBox)
		// 			.MinDesiredWidth(400.f)
		// 			.MinDesiredHeight(400.f)
		// 			[
		// 				ContentBrowser.CreateAssetPicker(AssetPickerConfig)
		// 			];
		
		// TArray<UConsoleVariablesAsset*> Presets;
		// FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		// IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		// AssetRegistry.ScanSynchronous( { "/Game/Presets" }, TArray<FString>{} );
		// AssetRegistry.SearchAllAssets( true );
		
		TArray<FAssetData> AssetData;
		// UAssetManager::Get().GetPrimaryAssetDataList( FPrimaryAssetType(UFactsPreset::StaticClass()->GetFName()), AssetData );
		IAssetRegistry::Get()->GetAssetsByClass( UFactsPreset::StaticClass()->GetClassPathName(), AssetData );

		FTextBuilder Builder;
		// AssetRegistry.GetAssetsByClass(UFactsPreset::StaticClass()->GetClassPathName(), AssetData);
		for (int i = 0; i < AssetData.Num(); i++) {
			// UStaticMesh* Object = Cast<UStaticMesh>(AssetData[i].GetAsset());
			Builder.AppendLine(AssetData[i].GetAsset()->GetName());
		}
				
		// TSharedRef<SWidget> PresetPicker = SNew( SComboBox< TSharedPtr < FAssetData > > );
		TSharedRef<SWidget> PresetPicker = SNew( STextBlock )
			.Text( Builder.ToText() );

		MenuBuilder.AddWidget( PresetPicker, FText(), true, false );
	}
	MenuBuilder.EndSection();
	
	return MenuBuilder.MakeWidget(  );
}

void SFactsEditor::HandleSearchTextChanged( const FText& SearchText )
{
	CurrentSearchText = SearchText;
	ExecuteSearch( SearchText );
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

void SFactsEditor::ExecuteSearch( const FText& SearchText )
{
	VisibleFactTreeItems.Empty(  );
	TArray<FFactTreeItemPtr> TempVisibleFacts;

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
		for ( FFactTreeItemPtr FactTreeItem : AllFactTreeItems )
		{
			if (ensure( FactTreeItem.IsValid() ) == false)
			{
				continue;
			}

			const bool bAllMatched = Algo::AnyOf( ActiveTogglesText, [Tag = FactTreeItem->TagNode->GetCompleteTagName().ToString()]( const FString& SearchText )
			{
				TArray<FString> Tokens;
				SearchText.ParseIntoArray( Tokens, TEXT("&") );

				
				return Algo::AllOf( Tokens, [Tag]( const FString& Token )
				{
					return Tag.Contains( Token );
				} );
			} );
		
			if ( bAllMatched )
			{
				TempVisibleFacts.Add( FactTreeItem );
			}
		}
	}
	else
	{
		TempVisibleFacts = AllFactTreeItems;
	}

	// second pass - find all visible item among previously filtered by search text
	if ( SearchText.IsEmpty() )
	{
		VisibleFactTreeItems = TempVisibleFacts;
		FactsTreeView->RequestTreeRefresh();
		return;
	}

	TArray<FString> Tokens;
	SearchText.ToString().ParseIntoArray( Tokens, TEXT("&") );
	
	for ( FFactTreeItemPtr FactTreeItem : TempVisibleFacts )
	{
		if (ensure( FactTreeItem.IsValid() ) == false)
		{
			continue;
		}

		bool bAllMatched = Algo::AllOf( Tokens, [Tag = FactTreeItem->TagNode->GetCompleteTagName().ToString()]( const FString& Token )
		{
			return Tag.Contains( Token );
		} );
		
		if ( bAllMatched )
		{
			VisibleFactTreeItems.Add( FactTreeItem );
		}
	}

	FactsTreeView->RequestTreeRefresh();
}

FReply SFactsEditor::HandleRemoveSearchToggle()
{
	CleanupSearchesMarkedForDelete();
	RefreshSearchToggles();

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

	ExecuteSearch( CurrentSearchText );

	return FReply::Handled();
}

FReply SFactsEditor::HandleSearchToggleClicked()
{
	ExecuteSearch( CurrentSearchText );
	return FReply::Handled();
}

void SFactsEditor::BuildFactTreeItems()
{
	AllFactTreeItems.Reset();

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	TSharedPtr< FGameplayTagNode > Node = Manager.FindTagNode( FFactTag::GetRootTag() );
	AllFactTreeItems.Add( BuildFactItem( Node ) );
	
	ExecuteSearch( FText::GetEmpty() );
}

FFactTreeItemPtr SFactsEditor::BuildFactItem( TSharedPtr< FGameplayTagNode > ThisNode )
{
	FFactTreeItemPtr NewItem = MakeShared< FFactTreeItem >(  );
	NewItem->Tag = FFactTag::ConvertChecked( ThisNode->GetCompleteTag() );
	NewItem->TagNode = ThisNode;
	NewItem->Children.Reserve( ThisNode->GetChildTagNodes().Num() );
	FactsTreeView->SetItemExpansion( NewItem, true );
	
	for ( TSharedPtr< FGameplayTagNode > Node : ThisNode->GetChildTagNodes() )
	{
		NewItem->Children.Add( BuildFactItem( Node ) );
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