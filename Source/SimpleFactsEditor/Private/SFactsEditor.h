// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactsDebuggerSettingsLocal.h"
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
	void InitItem();
	void HandleValueChanged(int32 NewValue);
	void HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type );

	TOptional< int32 > GetValue() const
	{
		return Value;
	}
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
	TSharedRef< SWidget > HandleGenerateOptionsMenu();

	// Searching and filtering
	void HandleSearchTextChanged( const FText& SearchText );
	void HandleSearchTextCommitted( const FText& SearchText, ETextCommit::Type Type );
	void FilterItems();

	// Options menu
	void HandleExpandAllClicked();
	void HandleCollapseAllClicked();

	// Items expansion
	void SetItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, TArray< FFactTreeItemPtr > FactItems, bool bShouldExpand, bool bPersistExpansion );
	void RestoreExpansionState();
	void SetDefaultItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, TArray< FFactTreeItemPtr > FactItems );

	static bool FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag, TArray< FFactTreeItemPtr >& OutPath );

	// Search toggles
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	void CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates );
	TArray< FSearchToggleState > GetSearchToggleStates();

	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();	

public:
	void BuildFactTreeItems();
	FFactTreeItemPtr BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode );

private:
	FString OnItemToStringDebug( FFactTreeItemPtr FactTreeItem ) const;

	// Settings
	void LoadSettings();
	void SaveSettings();
	
	void HandleSettingsChanged();
	void HandleShowRootTagClicked();
	void HandleShowFullNamesClicked();
	void HandleRemoveFavoritesClicked();
	void HandleShouldStackHierarchyHeadersClicked();
	void HandleOrientationChanged( EOrientation Orientation );

public:
	static TArray< FFactTag > FavoriteFacts;

private:
	TSharedPtr< SSplitter > Splitter;
	TSharedPtr<SFactsTreeView> FactsTreeView;
	TSharedPtr<SFactsTreeView> FavoriteFactsTreeView;
	
	FFactTreeItemPtr RootItem;
	FFactTreeItemPtr FilteredRootItem;
	FFactTreeItemPtr FavoritesRootItem;
	
	TSharedPtr< SSearchBox > SearchBox; 
	TSharedPtr< SComboButton > ComboButton;
	TSharedPtr< SComboButton > OptionsButton;

	TSharedPtr<SHorizontalBox> SearchesHBox;
	TSharedPtr<SWrapBox> SearchesContainer;
	TArray< TSharedRef< SFactsEditorSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;

	TObjectPtr< UFactsPreset > LoadedPreset;

	// Save expansion state for tag item. The expansion state does not persist between editor sessions. 
	static TSet< FFactTag > ExpandedStates;
	bool bPersistExpansionChange = true;

	bool bDisplayOnlyPinnedItems = false;
};
