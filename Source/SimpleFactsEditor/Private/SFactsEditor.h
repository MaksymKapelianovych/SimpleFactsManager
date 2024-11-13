// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactRuntimeDebugSubsystem.h"
#include "FactTypes.h"
#include "Widgets/SCompoundWidget.h"

class UFactsPreset;
class SFactsEditorSearchToggle;
class SWrapBox;

using FFactTreeItemRef = TSharedRef< struct FFactTreeItem >;
using FFactTreeItemPtr = TSharedPtr< struct FFactTreeItem >;

struct FFactTreeItem : public TSharedFromThis<FFactTreeItem>
{
	FFactTag Tag;
	TOptional< int32 > Value;

	TSharedPtr< FGameplayTagNode > TagNode;
	TArray< FFactTreeItemPtr > Children;

	FDelegateHandle Handle;

	~FFactTreeItem();

	void InitPIE();
	void HandleValueChanged(int32 NewValue);
	void HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type ) const;
};

/**
 * 
 */
class SIMPLEFACTSEDITOR_API SFactsEditor : public SCompoundWidget
{
	using SFactsTreeView = STreeView<TSharedPtr<FFactTreeItem>>;
	
public:
	SLATE_BEGIN_ARGS( SFactsEditor ) {}

	SLATE_END_ARGS()


	virtual void Construct(const FArguments& InArgs );
	TArray< FSearchToggleState > GetSearchToggleStates();

	void LoadFactsPreset( UFactsPreset* InPreset );

private:
	void HandleGameInstanceStarted();
	void InitItem( FFactTreeItemRef Item );
	
	TSharedRef<ITableRow> OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr String, const TSharedRef<STableViewBase>& TableViewBase );
	void OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray< FFactTreeItemPtr >& Children );

	TSharedRef<SWidget> HandleGeneratePresetsMenu() const;
	
	void HandleSearchTextChanged( const FText& SearchText );
	void HandleSearchTextCommitted( const FText& SearchText, ETextCommit::Type Type );
	void ExecuteSearch( const FText& SearchText );
	
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	void CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates );

	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();	

	void BuildFactTreeItems();
	FFactTreeItemPtr BuildFactItem( TSharedPtr< FGameplayTagNode > ThisNode );
	
	FString OnItemToStringDebug( FFactTreeItemPtr FactTreeItem ) const;
	
private:
	TSharedPtr<SFactsTreeView> FactsTreeView;
	TArray<FFactTreeItemPtr> AllFactTreeItems;
	TArray<FFactTreeItemPtr> VisibleFactTreeItems;

	TSharedPtr<SHorizontalBox> SearchesHBox;
	TSharedPtr<SWrapBox> SearchesContainer;
	TArray< TSharedRef< SFactsEditorSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;
};
