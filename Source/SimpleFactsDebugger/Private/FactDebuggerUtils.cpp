#include "FactDebuggerUtils.h"

#include "FactTypes.h"
#include "SFactDebugger.h"
#include "GameplayTagsManager.h"
#include "Algo/AllOf.h"

namespace Utils
{
	bool MatchSearchToggle( const TArray< FString >& SearchStrings, const FString& TagString )
	{
		if ( SearchStrings.IsEmpty() )
		{
			return true;
		}
		
		for ( const FString& SearchString : SearchStrings )
		{
			TArray< FString > Tokens;
			SearchString.ParseIntoArray( Tokens, TEXT( " " ) );

			auto Projection = [ TagString ]( const FString& Token ) { return TagString.Contains( Token ); };
			if ( Algo::AllOf( Tokens, Projection ) )
			{
				return true;
			}
		}

		return false;
	}

	bool MatchSearchBox( const TArray< FString >& SearchStrings, const FString& TagString )
	{
		if ( SearchStrings.IsEmpty() )
		{
			return true;
		}
		
		for ( const FString& SearchString : SearchStrings )
		{
			if ( TagString.Contains( SearchString ) == false )
			{
				return false;
			}
		}

		return true;
	}

	enum class ETagMatchType
	{
		None,
		Parent,
		Child,
		Full
	};
	
	ETagMatchType MatchFavorites( FFactTag CheckedTag )
	{
		bool bParentMatch = false;
		bool bChildMatch = false;
			
		for ( FFactTag FavoriteFact : SFactDebugger::FavoriteFacts )
		{
			if ( CheckedTag == FavoriteFact )
			{
				return ETagMatchType::Full;
			}

			if ( CheckedTag.MatchesTag( FavoriteFact ) )
			{
				bParentMatch = true;
			}
			else if ( FavoriteFact.MatchesTag( CheckedTag ) )
			{
				bChildMatch = true;
			}
		}

		if ( bParentMatch )
		{
			return ETagMatchType::Parent;
		}
		else if ( bChildMatch )
		{
			return ETagMatchType::Child;
		}

		return ETagMatchType::None;
	};
	
	void CopyItem( const FFactTreeItemPtr& SourceItem, TArray< FFactTreeItemPtr >& OutDestArray )
	{
		FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
		*NewItem = *SourceItem;
		NewItem->InitItem();
	}

	void FilterFavoriteFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );
	void FilterMainFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options );

	void CopyItemIfMainChildrenMatch( const FFactTreeItemPtr& SourceItem, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		TArray< FFactTreeItemPtr > FilteredChildren;
		FilterMainFactItemChildren( SourceItem->Children, FilteredChildren, Options );
		if ( FilteredChildren.Num() )
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
			NewItem->InitItem();
			NewItem->Children = FilteredChildren;
		}
	}

	void CopyItemIfFavoritesChildrenMatch( const FFactTreeItemPtr& SourceItem, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		TArray< FFactTreeItemPtr > FilteredChildren;
		FilterFavoriteFactItemChildren( SourceItem->Children, FilteredChildren, Options );
		if ( FilteredChildren.Num() )
		{
			FFactTreeItemPtr& NewItem = OutDestArray.Add_GetRef( MakeShared< FFactTreeItem >() );
			*NewItem = *SourceItem;
			NewItem->InitItem();
			NewItem->Children = FilteredChildren;
		}
	}
	
	void FilterFavoriteFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		auto MatchText = [ &Options ]( const FString& TagString )
		{
			return MatchSearchBox( Options.SearchBarStrings, TagString ) && MatchSearchToggle( Options.SearchToggleStrings, TagString );
		};

		for ( const FFactTreeItemPtr& SourceItem : SourceArray )
		{
			bool bNeedsFilteringByText = false;

			if ( Options.bShowOnlyDefinedFacts && Options.bIsPlaying )
			{
				if ( SourceItem->Children.Num() > 0 )
				{
					CopyItemIfFavoritesChildrenMatch( SourceItem, OutDestArray, Options );
					continue;
				}
				else if ( SourceItem->Value.IsSet() == false )
				{
					continue;
				}
			}

			switch ( MatchFavorites( SourceItem->Tag ) ) {
			case ETagMatchType::None: // early return
				continue;
			case ETagMatchType::Parent: // we only get here if option "Show only Defined Facts" is checked
				bNeedsFilteringByText = true;
				break;
			case ETagMatchType::Child: // straight to filtering children, even if this item matched - it is not favorite by itself
				CopyItemIfFavoritesChildrenMatch( SourceItem, OutDestArray, Options );
				continue;
			case ETagMatchType::Full:
				bNeedsFilteringByText = true;
				break;
			}

			if ( bNeedsFilteringByText )
			{
				if ( MatchText( SourceItem->Tag.ToString() ) ) // full match by favorites and by search texts
				{
					CopyItem( SourceItem, OutDestArray );
				}
			}
		}
	}

	void FilterMainFactItemChildren( const TArray< FFactTreeItemPtr >& SourceArray, TArray< FFactTreeItemPtr >& OutDestArray, const FFilterOptions& Options )
	{
		auto MatchText = [ &Options ]( const FString& TagString )
		{
			return MatchSearchBox( Options.SearchBarStrings, TagString ) && MatchSearchToggle( Options.SearchToggleStrings, TagString );
		};
			
		for ( const FFactTreeItemPtr& SourceItem : SourceArray )
		{
			bool bNeedsFilteringByText = false;

			if ( Options.bShowOnlyDefinedFacts && Options.bIsPlaying )
			{
				if ( SourceItem->Children.Num() > 0 ) // even if this item has value - children can be without value and therefore shoundn't be visible
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
					continue;
				}
				else if ( SourceItem->Value.IsSet() == false )
				{
					continue;
				}
			}

			switch ( MatchFavorites( SourceItem->Tag ) )
			{
			case ETagMatchType::None: // this is completely not a favorite fact, we can safely filter it by text
				bNeedsFilteringByText = true;
				break;
			case ETagMatchType::Parent: // parent tag is favorite, continue only if we are showing favorites in main tree
				if ( Options.bShowFavoritesInMainTree )
				{
					bNeedsFilteringByText = true;
					break;
				}
				else
				{
					continue;
				}
			case ETagMatchType::Child: // some child tag is favorite, if we are showing favorites in main tree - continue, otherwise - straight to filtering children
				if ( Options.bShowFavoritesInMainTree )
				{
					bNeedsFilteringByText = true;
					break;
				}
				else
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
					continue;
				}
			case ETagMatchType::Full: // tag is favorite, continue only if we are showing favorites in main tree
				if ( Options.bShowFavoritesInMainTree )
				{
					bNeedsFilteringByText = true;
					break;
				}
				else
				{
					continue;
				}
			}

			if ( bNeedsFilteringByText )
			{
				if ( MatchText( SourceItem->Tag.ToString() ) )
				{
					CopyItem( SourceItem, OutDestArray );
				}
				else
				{
					CopyItemIfMainChildrenMatch( SourceItem, OutDestArray, Options );
				}
			}
		}
	}


	void GetLeafTags( const TSharedPtr< FGameplayTagNode >& Node, TArray< TSharedPtr< FGameplayTagNode > >& OutLeafTagNodes )
	{
		if ( Node->GetChildTagNodes().IsEmpty() )
		{
			OutLeafTagNodes.Add( Node );
			return;
		}

		for ( TSharedPtr< FGameplayTagNode > Child : Node->GetChildTagNodes() )
		{
			GetLeafTags( Child, OutLeafTagNodes );
		}
	};
}
