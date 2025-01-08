// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactSearchToggle.h"

#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "Framework/Application/SlateApplication.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FactDebugger"

SFactSearchToggle::SFactSearchToggle()
	: CheckedColor()
	, UncheckedColor()
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactSearchToggle::Construct( const FArguments& InArgs, const FText& InButtonText )
{
	OnAltClicked = InArgs._OnAltClicked;
	OnRightButtonClicked = InArgs._OnRightButtonClicked;
	OnClickedOnce = InArgs._OnClickedOnce;
	bIsToggleChecked = InArgs._IsToggleChecked;
	
	SearchText = InButtonText;
	CheckedColor = USlateThemeManager::Get().GetColor( EStyleColor::Primary );
	UncheckedColor = USlateThemeManager::Get().GetColor( EStyleColor::Black );
	
	ChildSlot
	[
		SAssignNew( ToggleButtonPtr, SCheckBox )
		.Style( FAppStyle::Get(), "FilterBar.BasicFilterButton" )
		.ToolTipText( LOCTEXT( "SearchToggleTooltip", "Toggle this search" ) )
		.IsChecked( this, &SFactSearchToggle::GetCheckedState )
		.OnCheckStateChanged_Lambda( [ this ]( ECheckBoxState NewCheckBoxState )
		{
			SetIsButtonChecked( !GetIsToggleChecked() );
			
			if ( FSlateApplication::Get().GetModifierKeys().IsAltDown() && OnAltClicked.IsBound() )
			{
				SetIsMarkedForDelete( true );
				(void)OnAltClicked.Execute(  );
			}
			else if ( OnClickedOnce.IsBound() )
			{
				SetIsMarkedForDelete( false );
				(void)OnClickedOnce.Execute(  );
			}
		} )
		[
			SNew( STextBlock )
			.Text( FText::Format( INVTEXT( "{0}" ), SearchText ) )
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFactSearchToggle::~SFactSearchToggle()
{
	OnAltClicked.Unbind();
	OnRightButtonClicked.Unbind();
	OnClickedOnce.Unbind();

	ToggleButtonPtr.Reset();
}

FReply SFactSearchToggle::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

FReply SFactSearchToggle::OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	if ( InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnRightButtonClicked.IsBound() )
	{
		SetIsMarkedForDelete( true );
		return OnRightButtonClicked.Execute(  ).ReleaseMouseCapture();
	}
	return FReply::Handled().ReleaseMouseCapture();
}

#undef LOCTEXT_NAMESPACE