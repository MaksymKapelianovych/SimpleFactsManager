// Fill out your copyright notice in the Description page of Project Settings.


#include "SFactSearchBox.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFactSearchBox::Construct( const FArguments& InArgs )
{
	SSearchBox::Construct( SSearchBox::FArguments()
		.InitialText( InArgs._InitialText )
		.HintText( InArgs._HintText )
		.OnTextChanged( InArgs._OnTextChanged )
		.SelectAllTextWhenFocused( true )
	);

	OnSaveSearchClicked = InArgs._OnSaveSearchClicked;

	// + Button to save search
	Box->AddSlot()
		.VAlign( VAlign_Center )
		.HAlign( HAlign_Right )
		.AutoWidth()
		.Padding( 0, 0, 2, 0 )
		[
			// Button to save the currently occuring search 
			SNew( SButton )
			.ContentPadding( 0 )
			.ToolTipText( NSLOCTEXT( "FactEditor", "SaveSearch_ToolTip", "Save current search text" ) )
			.Visibility_Lambda( [ this ]()
			{
				// Only visible if there is a search active currently and the OnSaveSearchClicked delegate is bound
				return GetText().IsEmpty() || OnSaveSearchClicked.IsBound() == false ? EVisibility::Collapsed : EVisibility::Visible;
			})
			.ButtonStyle( FAppStyle::Get(), "HoverOnlyButton" )
			.OnClicked_Lambda( [ this ]()
			{
				(void)OnSaveSearchClicked.ExecuteIfBound( GetText() );
				return FReply::Handled();
			} )
			[
				SNew( SImage )
				.Image( FAppStyle::Get().GetBrush( "Icons.PlusCircle" ) )
				.ColorAndOpacity( FSlateColor::UseForeground() )
			]
		];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
