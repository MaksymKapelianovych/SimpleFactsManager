// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactsEditorSearchToggle.h"

#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "FactsEditor"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactsEditorSearchToggle::Construct( const FArguments& InArgs, const FText& InButtonText )
{
	OnAltClicked = InArgs._OnAltClicked;
	OnRightButtonClicked = InArgs._OnRightButtonClicked;
	OnClickedOnce = InArgs._OnClickedOnce;
	
	SearchText = InButtonText;
	CheckedColor = USlateThemeManager::Get().GetColor(EStyleColor::Primary);
	UncheckedColor = USlateThemeManager::Get().GetColor(EStyleColor::Black);
	
	ChildSlot
	[
		SNew( SBorder )
		.Padding( 1.0f )
		.BorderImage( FAppStyle::Get().GetBrush( "ContentBrowser.FilterBackground" ) )
		[
			SAssignNew( ToggleButtonPtr, SCheckBox )
			.Style( FAppStyle::Get(), "ContentBrowser.FilterButton" )
			.ToolTipText( LOCTEXT( "SearchToggleTooltip", "Toggle this search" ) )
			.IsChecked( this, &SFactsEditorSearchToggle::GetCheckedState )
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
				SNew( SHorizontalBox )

				// -----------------------------------------------------------------------------------------------------
				// Image that shows if this search is checked
				+SHorizontalBox::Slot()
				.VAlign( VAlign_Center )
				.AutoWidth()
				[
					SNew( SImage )
					.Image( FAppStyle::Get().GetBrush( "ContentBrowser.FilterImage" ) )
					.ColorAndOpacity_Lambda( [ this ]()
					{
						return GetIsToggleChecked() ? CheckedColor : UncheckedColor;
					})
				]

				// -----------------------------------------------------------------------------------------------------
				// Search text
				+SHorizontalBox::Slot()
				.Padding( 2.f )
				.VAlign( VAlign_Center )
				[
					SNew( STextBlock )
					.Text( FText::Format( INVTEXT( "{0}" ), SearchText ) )
				]
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFactsEditorSearchToggle::~SFactsEditorSearchToggle()
{
	OnAltClicked.Unbind();
	OnRightButtonClicked.Unbind();
	OnClickedOnce.Unbind();

	ToggleButtonPtr.Reset();
}

FReply SFactsEditorSearchToggle::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

FReply SFactsEditorSearchToggle::OnMouseButtonUp( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	if ( InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnRightButtonClicked.IsBound() )
	{
		SetIsMarkedForDelete( true );
		return OnRightButtonClicked.Execute(  ).ReleaseMouseCapture();
	}
	return FReply::Handled().ReleaseMouseCapture();
}

#undef LOCTEXT_NAMESPACE