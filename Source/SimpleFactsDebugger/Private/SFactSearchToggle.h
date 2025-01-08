// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/SlateDelegates.h"

class SCheckBox;

/**
 * 
 */
class SFactSearchToggle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SFactSearchToggle )
		: _IsToggleChecked( false )
		{}
		SLATE_EVENT( FOnClicked, OnAltClicked )
		SLATE_EVENT( FOnClicked, OnRightButtonClicked )
		SLATE_EVENT( FOnClicked, OnClickedOnce )

		SLATE_ARGUMENT( bool, IsToggleChecked )
	SLATE_END_ARGS()

	SFactSearchToggle();
	void Construct( const FArguments& InArgs, const FText& InButtonText );

	virtual ~SFactSearchToggle() override;
	
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override;

	
	[[nodiscard]] bool GetIsToggleChecked() const {	return bIsToggleChecked; }
	[[nodiscard]] ECheckBoxState GetCheckedState() const { return GetIsToggleChecked() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }
	void SetIsButtonChecked(const bool bNewIsButtonChecked) { bIsToggleChecked = bNewIsButtonChecked; }

	[[nodiscard]] bool GetIsMarkedForDelete() const { return bIsMarkedForDelete; }
	void SetIsMarkedForDelete(const bool bNewMark) { bIsMarkedForDelete = bNewMark; }

	const FText& GetSearchText() { return SearchText; }

private:
	FOnClicked OnAltClicked;
	FOnClicked OnRightButtonClicked;
	FOnClicked OnClickedOnce;

	TSharedPtr< SCheckBox > ToggleButtonPtr;
	bool bIsToggleChecked = true;

	/** When clicked in a special manner, this search button will be marked for deletion
	 * The responsibility is on external classes to remove the button from the UI the external class controls. */
	bool bIsMarkedForDelete = false;
	
	FLinearColor CheckedColor;
	FLinearColor UncheckedColor;
	
	FText SearchText;
};

using SFactSearchToggleRef = TSharedRef< SFactSearchToggle >;
using SFactSearchTogglePtr = TSharedPtr< SFactSearchToggle >;