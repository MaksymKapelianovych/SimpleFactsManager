// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditor.h"

#include "FactSubsystem.h"
#include "SFactsEditorSearchToggle.h"
#include "SlateOptMacros.h"
#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "FactsEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

FFactTreeItem::~FFactTreeItem()
{
	if (GameInstance.IsValid() && GameInstance->GetWorld())
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( GameInstance->GetWorld() );
		FactSubsystem.GetOnFactValueChangedDelegate( Tag ).Remove( Handle );
	}
}

void FFactTreeItem::HandleValueChanged( int32 NewValue )
{
	Value = NewValue;
}

void FFactTreeItem::HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type ) const
{
	if (GameInstance.Get())
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( GameInstance->GetWorld() );
		FactSubsystem.ChangeFactValue( Tag, NewValue, EFactValueChangeType::Set );
	}
}

void SFactsEditor::Construct( const FArguments& InArgs, TWeakObjectPtr< UGameInstance > InGameInstance, TArray< FSearchToggleState > SearchToggleStates )
{
	GameInstance = InGameInstance;
	
	ChildSlot
	[
		SNew(SVerticalBox)

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
				.ItemHeight( 24.f )
				.TreeItemsSource( &VisibleFactTreeItems )
				.OnItemToString_Debug( this, &SFactsEditor::OnItemToStringDebug )
				.OnGenerateRow( this, &SFactsEditor::OnGenerateWidgetForFactsTreeView )
				.OnGetChildren( this, &SFactsEditor::OnGetChildren)
				.HeaderRow
				(
					SNew( SHeaderRow )
					
					+ SHeaderRow::Column( "FactTag" )
					.DefaultLabel( LOCTEXT( "FactTag", "Tag" ) )
					.FillWidth( .5f )

					+ SHeaderRow::Column( "FactValue" )
					.DefaultLabel( LOCTEXT( "FactValue", "Value" ) )
				)
			]
		]
	];

	CreateDefaultSearchToggles( SearchToggleStates );
	UpdateFactTreeItems();
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
			// FactsEditor = InFactsEditor;
			SMultiColumnTableRow::Construct( FSuperRowType::FArguments(), InOwnerTable );
			
			SetBorderBackgroundColor( FSlateColor{FLinearColor::Green} );
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
						.Text( FText::FromString( Item->Tag.ToString() ) )
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

			const bool bAllMatched = Algo::AnyOf( ActiveTogglesText, [Tag = FactTreeItem->Tag.GetTagName().ToString()]( const FString& SearchText )
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

		bool bAllMatched = Algo::AllOf( Tokens, [Tag = FactTreeItem->Tag.GetTagName().ToString()]( const FString& Token )
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

void SFactsEditor::UpdateFactTreeItems()
{
	AllFactTreeItems.Reset();

	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	FGameplayTagContainer AllFactTags = Manager.RequestGameplayTagChildren( FFactTag::GetRootTag() );

	
	UFactSubsystem* FactSubsystem = nullptr;
	// UWorld* World = nullptr;

	// if(GEditor && GEditor->PlayWorld)
	// {
	// 	World = GEditor->PlayWorld;
	// }
	
	// if (World)
	// {
		FactSubsystem = &UFactSubsystem::Get( GameInstance->GetWorld() );
	// }

	for (const FGameplayTag& FactTag : AllFactTags )
	{
		FFactTreeItemPtr NewItem = MakeShared< FFactTreeItem >(  );
		NewItem->Tag = FFactTag::ConvertChecked( FactTag );
		NewItem->GameInstance = GameInstance;
		if (FactSubsystem)
		{
			int32 Value;
			if (FactSubsystem->TryGetFactValue( NewItem->Tag, Value ))
			{
				NewItem->Value = Value;
			}
			
			NewItem->Handle = FactSubsystem->GetOnFactValueChangedDelegate( NewItem->Tag ).AddSP( NewItem.ToSharedRef(), &FFactTreeItem::HandleValueChanged );
		}
		
		AllFactTreeItems.Add( NewItem );
	}

	ExecuteSearch( FText::GetEmpty() );
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