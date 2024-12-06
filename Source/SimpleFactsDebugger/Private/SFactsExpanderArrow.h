// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ITableRow;

/**
 * Copy of SStateTreeExpanderArrow (it is smaller that default SExpanderArrow and looks the same).
 * This class exists only because default expander arrow draws vertical line for top-level items (seems like a bug), and I don't like it
 */
class SFactsExpanderArrow : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SFactsExpanderArrow )
		: _StyleSet( &FCoreStyle::Get() )
		, _IndentAmount( 10 )
		, _BaseIndentLevel( 0 )
	{ }
		SLATE_ARGUMENT( const ISlateStyle*, StyleSet )
		/** How many Slate Units to indent for every level of the tree. */
		SLATE_ATTRIBUTE( float, IndentAmount )
		/** The level that the root of the tree should start (e.g. 2 will shift the whole tree over by `IndentAmount*2`) */
		SLATE_ATTRIBUTE( int32, BaseIndentLevel )
	SLATE_END_ARGS()
	
	void Construct( const FArguments& InArgs, const TSharedPtr< ITableRow >& TableRow );

protected:

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	/** Invoked when the expanded button is clicked (toggle item expansion) */
	FReply OnArrowClicked();

	/** @return Visible when has children; invisible otherwise */
	EVisibility GetExpanderVisibility() const;

	/** @return the margin corresponding to how far this item is indented */
	FMargin GetExpanderPadding() const;

	/** @return the name of an image that should be shown as the expander arrow */
	const FSlateBrush* GetExpanderImage() const;

	TWeakPtr< ITableRow > OwnerRowPtr;

	/** A reference to the expander button */
	TSharedPtr< SButton > ExpanderArrow;

	/** The slate style to use */
	const ISlateStyle* StyleSet;

	/** The amount of space to indent at each level */
	TAttribute< float > IndentAmount;

	/** The level in the tree that begins the indention amount */
	TAttribute< int32 > BaseIndentLevel;
};
