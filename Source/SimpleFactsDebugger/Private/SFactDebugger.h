// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactDebuggerSettingsLocal.h"
#include "FactTypes.h"
#include "Widgets/SCompoundWidget.h"

class UFactPreset;
class SFactSearchToggle;
class SWrapBox;
class SFactSearchBox;

using FFactTreeItemRef = TSharedRef< struct FFactTreeItem >;
using FFactTreeItemPtr = TSharedPtr< struct FFactTreeItem >;

struct FFactTreeItem : public TSharedFromThis< FFactTreeItem >
{
	FFactTag Tag;
	FName SimpleTagName;
	TArray< FFactTreeItemPtr > Children;

	TOptional< int32 > Value;
	float ValueChangedTime = 0;

	~FFactTreeItem();

	void StartPlay();
	void EndPlay();
	void InitItem();
	
	void HandleValueChanged( int32 NewValue );
	void HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type );

	DECLARE_DELEGATE_TwoParams( FOnFactItemValueChanged, FFactTag, int32 )
	FOnFactItemValueChanged OnFactItemValueChanged;
	FDelegateHandle Handle;
};


class SFactDebugger : public SCompoundWidget
{
	using SFactsTreeView = STreeView< TSharedPtr< FFactTreeItem > >;
	
public:
	SLATE_BEGIN_ARGS( SFactDebugger ) {}
		SLATE_ARGUMENT( bool, bIsGameStarted )
	SLATE_END_ARGS()


	virtual void Construct( const FArguments& InArgs );
	virtual ~SFactDebugger() override;

	// Todo: maybe move to utils
	static int32 CountAllFilteredItems( FFactTreeItemPtr ParentNode );
	static int32 CountAllFavoriteItems( FFactTreeItemPtr ParentNode, bool bIsParentFavorite );
	
private:
	// Play started
	void HandleGameInstanceStarted();
	void HandleGameInstanceEnded();
	void InitItem( FFactTreeItemPtr Item );
	void ResetItem( FFactTreeItemPtr Item );

	TSharedRef< SWidget > CreateLeftToolBar();
	TSharedRef< SWidget > CreateRightToolBar();
	
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
	void GenerateCommonContextMenu( FMenuBuilder& MenuBuilder, bool bIsFavoritesTree );
	TSharedPtr< SWidget > HandleGenerateMainContextMenu();
	TSharedPtr< SWidget > HandleGenerateFavoritesContextMenu();
	
	void ClearFavoritesRecursive( FFactTreeItemPtr Item );
	bool HasFavoritesRecursive( FFactTreeItemPtr Item );
	void PostFavoritesChanged();

	// Searching and filtering
	void HandleSearchTextChanged( const FText& SearchText );
	void HandleSaveSearchClicked( const FText& SearchText );
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
	TSharedRef< SFactSearchToggle > ConstructSearchToggle( const FText& InSearchText, bool bInChecked = false );
	
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();
	
	TArray< FSearchToggleState > GetSearchToggleStates();
	bool IsAnySearchToggleActive() const;

	// Build items
	void BuildFactTreeItems();
	FFactTreeItemPtr BuildFactItem( FFactTreeItemPtr ParentNode, TSharedPtr< FGameplayTagNode > ThisNode );
	void RebuildFactTreeItems();
	void HandleFactValueChanged( FFactTag FactTag, int32 NewValue );
	
	// Settings
	void LoadSettings();
	void SaveSettings();
	
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
	
	TSharedPtr< SFactSearchBox > SearchBox; 
	TSharedPtr< SComboButton > OptionsButton;

	TSharedPtr< SHorizontalBox > SearchesHBox;
	TSharedPtr< SWrapBox > SearchesContainer;
	TArray< TSharedRef< SFactSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;

	// Save expansion state for tag item. The expansion state does not persist between editor sessions. 
	static TSet< FFactTag > MainExpandedFacts;
	static TSet< FFactTag > FavoritesExpandedFacts;
	bool bPersistExpansionChange = true;

	bool bDisplayOnlyPinnedItems = false;

#if WITH_EDITOR
	FDelegateHandle TagChangedHandle;
#endif

	bool bIsPlaying = false;
};
