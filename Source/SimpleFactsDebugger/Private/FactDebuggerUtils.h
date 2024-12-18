#pragma once

using FFactTreeItemPtr = TSharedPtr< struct FFactTreeItem >;
struct FGameplayTagNode;

namespace Utils
{
	struct FFilterOptions
	{
		TArray< FString > SearchToggleStrings;
		TArray< FString > SearchBarStrings;
		
		bool bIsPlaying;
		bool bShowOnlyDefinedFacts;
		bool bShowFavoritesInMainTree;
	};

	void FilterFavoritesFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );
	void FilterMainFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );

	void GetLeafTags( const TSharedPtr< FGameplayTagNode >& Node, TArray< TSharedPtr< FGameplayTagNode > >& OutLeafTagNodes );
}
