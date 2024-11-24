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

enum EFactFilterMode
{
	SearchBox, // acts as AND
	SearchToggles // acts as OR
};

/**
 * 
 */
class SIMPLEFACTSEDITOR_API SFactsEditor : public SCompoundWidget
{
	using SFactsTreeView = STreeView<TSharedPtr<FFactTreeItem>>;
	
public:
	SLATE_BEGIN_ARGS( SFactsEditor ) {}
		SLATE_ARGUMENT( bool, bIsGameStarted )
	SLATE_END_ARGS()


	virtual void Construct(const FArguments& InArgs );
	TArray< FSearchToggleState > GetSearchToggleStates();

	void LoadFactsPreset( UFactsPreset* InPreset );
	void LoadFactsPresetRecursive( UFactsPreset* InPreset, const FFactTreeItemPtr& FactItem ) const;

private:
	void HandleGameInstanceStarted();
	void InitItem( FFactTreeItemRef Item );
	
	TSharedRef< ITableRow > OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase );
	TSharedRef< ITableRow > HandleGeneratePinnedTreeRow( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase);
	void OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray< FFactTreeItemPtr >& Children );
	void HandleItemExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded );

	TSharedRef<SWidget> HandleGeneratePresetsMenu();
	
	void HandleSearchTextChanged( const FText& SearchText );
	void HandleSearchTextCommitted( const FText& SearchText, ETextCommit::Type Type );
	void FilterItems();
	void FilterFactItemChildren( TArray< FString > FilterStrings, EFactFilterMode FilterMode, TArray< FFactTreeItemPtr>& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray );

	void ExpandAll( TArray< FFactTreeItemPtr > FactItems );
	void RestoreExpansionState();

	static bool FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag, TArray< FFactTreeItemPtr >& OutPath );
	
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	void CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates );

	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();	

	void BuildFactTreeItems();
	FFactTreeItemPtr BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode );
	
	FString OnItemToStringDebug( FFactTreeItemPtr FactTreeItem ) const;
	
private:
	TSharedPtr<SFactsTreeView> FactsTreeView;
	FFactTreeItemPtr RootItem;
	FFactTreeItemPtr FilteredRootItem;
	
	TSharedPtr<SComboButton> ComboButton;

	TSharedPtr<SHorizontalBox> SearchesHBox;
	TSharedPtr<SWrapBox> SearchesContainer;
	TArray< TSharedRef< SFactsEditorSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;

	TObjectPtr< UFactsPreset > LoadedPreset;

	// Save expansion state for tag item. The expansion state does not persist between editor sessions. 
	static TArray< FFactTag > CollapsedStates;
	bool bIsRestoringExpansion = false;
};
