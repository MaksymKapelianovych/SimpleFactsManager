// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FFactTreeItem : public TSharedFromThis<FFactTreeItem>
{
	FFactTag Tag;
	TOptional< int32 > Value;

	TArray< TSharedPtr< FFactTreeItem > > Children;

	FDelegateHandle Handle;
	TWeakObjectPtr<UGameInstance> GameInstance;

	~FFactTreeItem();

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


	virtual void Construct(const FArguments& InArgs, TWeakObjectPtr<UGameInstance> InGameInstance);

private:
	TSharedRef<ITableRow> OnGenerateWidgetForFactsTreeView( TSharedPtr<FFactTreeItem> String, const TSharedRef<STableViewBase>& TableViewBase );
	void OnGetChildren( TSharedPtr<FFactTreeItem> FactTreeItem, TArray<TSharedPtr<FFactTreeItem>>& Children );

	void UpdateFactTreeItems();
	
private:
	TSharedPtr<SFactsTreeView> FactsTreeView;
	TArray<TSharedPtr<FFactTreeItem>> FactTreeItems;

	TWeakObjectPtr<UGameInstance> GameInstance;
};
