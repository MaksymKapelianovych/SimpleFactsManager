// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "FactSearchBox"

/**
 * Subclass of SSearchBox, to allow adding the save search button to the Horizontal Box internal to SEditableTextBox
 */
class SIMPLEFACTSDEBUGGER_API SFactSearchBox : public SSearchBox
{
public:
	DECLARE_DELEGATE_OneParam( FOnSaveSearchClicked, const FText& )

	SLATE_BEGIN_ARGS( SFactSearchBox )
		: _HintText( LOCTEXT( "SearchHint", "Search" ) )
		, _InitialText()
		, _OnTextChanged()
	{ }

		/** The text displayed in the SearchBox when no text has been entered */
		SLATE_ATTRIBUTE( FText, HintText )

		/** The text displayed in the SearchBox when it's created */
		SLATE_ATTRIBUTE( FText, InitialText )
		
		/** Invoked whenever the text changes */
		SLATE_EVENT( FOnTextChanged, OnTextChanged )

		/** Delegate for when the Plus icon is clicked */
		SLATE_EVENT( FOnSaveSearchClicked, OnSaveSearchClicked )

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct( const FArguments& InArgs );

private:
	
	/** Delegate for when the Plus icon is clicked */
	FOnSaveSearchClicked OnSaveSearchClicked;
};
