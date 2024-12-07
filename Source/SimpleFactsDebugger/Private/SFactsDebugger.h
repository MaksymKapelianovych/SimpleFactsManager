// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactsDebuggerSettingsLocal.h"
#include "FactTypes.h"
#include "Widgets/SCompoundWidget.h"

class UFactsPreset;
class SFactsSearchToggle;
class SWrapBox;

using FFactTreeItemRef = TSharedRef< struct FFactTreeItem >;
using FFactTreeItemPtr = TSharedPtr< struct FFactTreeItem >;

struct FFactTreeItem : public TSharedFromThis< FFactTreeItem >
{
	FFactTag Tag;
	TOptional< int32 > Value;

	TSharedPtr< FGameplayTagNode > TagNode;
	TArray< FFactTreeItemPtr > Children;

	FDelegateHandle Handle;

	~FFactTreeItem();

	void InitPIE();
	void InitItem();
	void HandleValueChanged( int32 NewValue );
	void HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type );

	TOptional< int32 > GetValue() const
	{
		return Value;
	}

	DECLARE_DELEGATE_OneParam( FOnFactItemValueChanged, int32 )
	FOnFactItemValueChanged OnFactItemValueChanged;

	bool bFactManualChanging = false;
};


class SFactsDebugger : public SCompoundWidget
{
	using SFactsTreeView = STreeView< TSharedPtr< FFactTreeItem > >;
	
public:
	SLATE_BEGIN_ARGS( SFactsDebugger ) {}
		SLATE_ARGUMENT( bool, bIsGameStarted )
	SLATE_END_ARGS()


	virtual void Construct( const FArguments& InArgs );
	virtual ~SFactsDebugger() override;

	// Todo: remove from widget
	void LoadFactsPreset( UFactsPreset* InPreset );
	void LoadFactsPresetRecursive( UFactsPreset* InPreset, const FFactTreeItemPtr& FactItem ) const;

	// Todo: maybe move to utils
	static int32 CountAllFilteredItems( FFactTreeItemPtr ParentNode );
	static int32 CountAllFavoriteItems( FFactTreeItemPtr ParentNode, bool bIsParentFavorite );
	
private:
	// Play started
	void HandleGameInstanceStarted();
	void InitItem( FFactTreeItemRef Item );
	
	// Create tree widgets
	TSharedRef< SWidget > CreateTreeLabel( const FText& InLabel ) const;
	TSharedRef< SWidget > CreateFactsTree( bool bIsFavoritesTree );
	TSharedRef< SHeaderRow > CreateHeaderRow( bool bIsFavoritesTree ) const;
	TSharedRef< SWidget > CreateFilterStatusWidget( bool bIsFavoritesTree ) const;
	
	TSharedRef< ITableRow > OnGenerateWidgetForFactsTreeView( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase );
	TSharedRef< ITableRow > HandleGeneratePinnedTreeRow( FFactTreeItemPtr FactTreeItem, const TSharedRef< STableViewBase >& TableViewBase );
	void OnGetChildren( FFactTreeItemPtr FactTreeItem, TArray< FFactTreeItemPtr >& Children );
	void HandleExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded, bool bRecursive, bool bIsFavoritesTree );
	
	FText GetFilterStatusText( bool bIsFavoritesTree ) const;
	FSlateColor GetFilterStatusTextColor( bool bIsFavoritesTree ) const;

	TSharedRef< SWidget > HandleGeneratePresetsMenu();
	TSharedRef< SWidget > HandleGenerateOptionsMenu();
	TSharedPtr< SWidget > HandleGenerateMainContextMenu();
	TSharedPtr< SWidget > HandleGenerateFavoritesContextMenu();
	
	void ClearFavoritesRecursive( FFactTreeItemPtr Item );
	bool HasFavoritesRecursive( FFactTreeItemPtr Item );

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
	void SetDefaultItemsExpansion( TSharedPtr< SFactsTreeView > TreeView, const TArray< FFactTreeItemPtr >& FactItems, const TSet< FFactTag >& ExpandedFacts );

	static bool FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag, TArray< FFactTreeItemPtr >& OutPath );

	// Search toggles
	void CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates );
	TSharedRef< SFactsSearchToggle > ConstructSearchToggle( const FText& InSearchText, bool bInChecked = false );
	
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();
	
	TArray< FSearchToggleState > GetSearchToggleStates();
	bool IsAnySearchToggleActive() const;
	
	void BuildFactTreeItems();
	FFactTreeItemPtr BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode );
	void RebuildFactTreeItems();
	
	// Settings
	void LoadSearchToggles();
	void SaveSearchToggles();
	void LoadFavorites();
	void SaveFavorites();
	
	void HandleOrientationChanged( EOrientation Orientation );

public:
	static TArray< FFactTag > FavoriteFacts;

private:
	TSharedPtr< SSplitter > Splitter;
	TSharedPtr< SFactsTreeView > FactsTreeView;
	TSharedPtr< SFactsTreeView > FavoriteFactsTreeView;
	
	FFactTreeItemPtr RootItem;
	FFactTreeItemPtr FilteredRootItem;
	FFactTreeItemPtr FavoritesRootItem;
	
	TSharedPtr< SSearchBox > SearchBox; 
	TSharedPtr< SComboButton > ComboButton;
	TSharedPtr< SComboButton > OptionsButton;

	TSharedPtr< SHorizontalBox > SearchesHBox;
	TSharedPtr< SWrapBox > SearchesContainer;
	TArray< TSharedRef< SFactsSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;

	TObjectPtr< UFactsPreset > LoadedPreset;

	// Save expansion state for tag item. The expansion state does not persist between editor sessions. 
	static TSet< FFactTag > MainExpandedFacts;
	static TSet< FFactTag > FavoritesExpandedFacts;
	bool bPersistExpansionChange = true;

	bool bDisplayOnlyPinnedItems = false;

	FDelegateHandle TagChangedHandle;
};
