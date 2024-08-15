// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditor.h"

#include "FactSubsystem.h"
#include "SlateOptMacros.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "SFactsEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

FFactTreeItem::~FFactTreeItem()
{
	if (GameInstance.Get())
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

void SFactsEditor::Construct( const FArguments& InArgs, TWeakObjectPtr<UGameInstance> InGameInstance )
{
	GameInstance = InGameInstance;
	
	ChildSlot
	[
		SNew(SVerticalBox)

		// -------------------------------------------------------------------------------------------------------------
		// SearchBar
		
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FactSearch", "Here will be search widget"))
			]
		]

		// -------------------------------------------------------------------------------------------------------------
		// FactsTree

		+ SVerticalBox::Slot()
		.FillHeight( 1.f )
		[
			SNew( SBorder )
			.BorderImage( FAppStyle::GetBrush( "ToolPanel.GroupBorder" ) )
			[
				SAssignNew( FactsTreeView, SFactsTreeView )
				.ItemHeight( 24.f )
				.TreeItemsSource( &FactTreeItems )
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

	UpdateFactTreeItems();

	// UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	// Manager.OnGameplayTagLoadedDelegate.AddLambda( [this](const FGameplayTag& LoadedTag)
	// {
	// 	if ( FFactTag::TryConvert( LoadedTag ).IsValid() )
	// 	{
	// 		UpdateFactTreeItems();
	// 	}
	// } );
}

TSharedRef<ITableRow> SFactsEditor::OnGenerateWidgetForFactsTreeView( TSharedPtr<FFactTreeItem> InItem,
	const TSharedRef<STableViewBase>& TableViewBase )
{
	class SFactTreeItem : public SMultiColumnTableRow< TSharedPtr< FFactTreeItem > >
	{
	public:
		SLATE_BEGIN_ARGS( SFactTreeItem ) {}
		SLATE_END_ARGS()

		void Construct( const FArguments& InArgs, const TSharedRef< SFactsTreeView > InOwnerTable, TSharedPtr< FFactTreeItem > InItem )
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
		// void HandleValueCommited( int32 Value, ETextCommit::Type Type ) const
		// {
		// 	Item->HandleValueChanged(  )
		// }

		TSharedPtr< FFactTreeItem > Item;
		// TWeakPtr<SFactsEditor> FactsEditor;
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

void SFactsEditor::OnGetChildren( TSharedPtr<FFactTreeItem> FactTreeItem, TArray<TSharedPtr<FFactTreeItem>>& Children )
{
	if ( FactTreeItem.IsValid() )
	{
		Children.Append( FactTreeItem->Children );
	}
}

void SFactsEditor::UpdateFactTreeItems()
{
	FactTreeItems.Reset();

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
		TSharedPtr< FFactTreeItem > NewItem = MakeShared< FFactTreeItem >(  );
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
		
		FactTreeItems.Add( NewItem );
	}

	FactsTreeView->RequestTreeRefresh();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


#undef LOCTEXT_NAMESPACE