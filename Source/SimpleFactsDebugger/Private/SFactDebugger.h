// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactDebuggerSettingsLocal.h"
#include "FactTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UFactPreset;
class SFactSearchToggle;
class SWrapBox;
class SFactSearchBox;
class SComboButton;

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
	void InitItem( bool bPlayAnimation = false );
	
	void HandleValueChanged( int32 NewValue );
	void HandleNewValueCommited( int32 NewValue, ETextCommit::Type Type ) const;

	DECLARE_MULTICAST_DELEGATE_TwoParams( FOnFactItemValueChanged, FFactTag, int32 )
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
	
private:
	// Play started
	void HandleGameInstanceStarted();
	void HandleGameInstanceEnded();
	void InitItem( const FFactTreeItemPtr& Item );
	void ResetItem( const FFactTreeItemPtr& Item );

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
	void HandleMainExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded, bool bRecursive );
	void HandleFavoritesExpansionChanged( FFactTreeItemPtr FactTreeItem, bool bInExpanded, bool bRecursive );
	
	FText GetFilterStatusText( bool bIsFavoritesTree ) const;
	FSlateColor GetFilterStatusTextColor( bool bIsFavoritesTree ) const;

	TSharedRef< SWidget > HandleGeneratePresetsMenu();
	TSharedRef< SWidget > HandleGenerateOptionsMenu();
	void GenerateCommonContextMenu( FMenuBuilder& MenuBuilder, bool bIsFavoritesTree );
	TSharedPtr< SWidget > HandleGenerateMainContextMenu();
	TSharedPtr< SWidget > HandleGenerateFavoritesContextMenu();
	
	void ClearFavoritesRecursive( const FFactTreeItemPtr& Item ) const;
	bool HasFavoritesRecursive( const FFactTreeItemPtr& Item ) const;
	void PostFavoritesChanged();

	// Searching and filtering
	void HandleSearchTextChanged( const FText& SearchText );
	void HandleSaveSearchClicked( const FText& SearchText );
	void FilterItems();

	static int32 CountAllMainItems( const FFactTreeItemPtr& ParentNode );
	static int32 CountAllFavoriteItems( const FFactTreeItemPtr& ParentNode, bool bIsParentFavorite );

	// Options menu
	void HandleExpandAllClicked( bool bExpandMain, bool bExpandFavorites );
	void HandleCollapseAllClicked( bool bCollapseMain, bool bCollapseFavorites );

	// Items expansion
	void SetItemsExpansion( const TSharedPtr< SFactsTreeView >& TreeView, const TArray< FFactTreeItemPtr >& FactItems, bool bShouldExpand, bool bPersistExpansion );
	void SetDefaultMainItemsExpansion( const TArray< FFactTreeItemPtr >& FactItems );
	void SetDefaultFavoriteItemsExpansion( const TArray< FFactTreeItemPtr >& FactItems );

	static bool FindItemByTagRecursive( const FFactTreeItemPtr& Item, const FFactTag Tag, TArray< FFactTreeItemPtr >& OutPath );

	// Search toggles
	void CreateDefaultSearchToggles( TArray< FSearchToggleState > SearchToggleStates );
	TSharedRef< SFactSearchToggle > ConstructSearchToggle( const FText& InSearchText, bool bInChecked = false );
	
	FReply HandleRemoveSearchToggle();
	void CleanupSearchesMarkedForDelete();
	void RefreshSearchToggles();
	FReply HandleClearTogglesClicked();
	FReply HandleSearchToggleClicked();
	
	TArray< FSearchToggleState > GetSearchToggleStates() const;
	bool IsAnySearchToggleActive() const;

	// Build items
	void BuildFactTreeItems( bool bPlayAnimation = false );
	FFactTreeItemPtr BuildFactItem( const FFactTreeItemPtr& ParentNode, const TSharedPtr< FGameplayTagNode >& ThisNode, bool bPlayAnimation );
	void RebuildFactTreeItems( bool bPlayAnimation = false );
	void HandleFactValueChanged( FFactTag FactTag, int32 NewValue );
	
	// Settings
	void LoadSettings();
	void SaveSettings() const;
	
	void HandleOrientationChanged( EOrientation Orientation ) const;

public:
	static TArray< FFactTag > FavoriteFacts;

private:
	TSharedPtr< SSplitter > Splitter;
	TSharedPtr< SFactsTreeView > MainTreeView;
	TSharedPtr< SFactsTreeView > FavoriteTreeView;
	
	FFactTreeItemPtr RootItem;
	FFactTreeItemPtr MainTreeItem;
	FFactTreeItemPtr FavoritesTreeItem;

	int32 AllMainFactsCount = 0;
	int32 AllFavoriteFactsCount = 0;
	
	int32 CurrentMainFactsCount = 0;
	int32 CurrentFavoriteFactsCount = 0;
	
	TSharedPtr< SFactSearchBox > SearchBox; 
	TSharedPtr< SComboButton > OptionsButton;

	TSharedPtr< SHorizontalBox > SearchesHBox;
	TSharedPtr< SWrapBox > SearchesContainer;
	TArray< TSharedRef< SFactSearchToggle > > CurrentSearchToggles;
	FText CurrentSearchText;

	// Save expansion state for tag item. The expansion state does not persist between editor sessions. 
	static TSet< FFactTag > MainExpandedFacts;
	static TSet< FFactTag > FavoriteCollapsedFacts;
	bool bPersistExpansionChange = true;

	bool bDisplayOnlyPinnedItems = false;

#if WITH_EDITOR
	FDelegateHandle TagChangedHandle;
#endif
	FDelegateHandle FactsLoadedHandle;

	bool bIsPlaying = false;
};
