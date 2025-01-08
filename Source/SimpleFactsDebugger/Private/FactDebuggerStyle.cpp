// Fill out your copyright notice in the Description page of Project Settings.


#include "FactDebuggerStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

TSharedPtr< FFactDebuggerStyle > FFactDebuggerStyle::StyleInstance;

void FFactDebuggerStyle::Register()
{
	check( StyleInstance.IsValid() == false );
	StyleInstance = MakeShared< FFactDebuggerStyle >();
	FSlateStyleRegistry::RegisterSlateStyle( *StyleInstance );
}

void FFactDebuggerStyle::Unregister()
{
	check( StyleInstance.IsValid() );
	FSlateStyleRegistry::UnRegisterSlateStyle( *StyleInstance );
	StyleInstance.Reset();
}

FFactDebuggerStyle& FFactDebuggerStyle::Get()
{
	check( StyleInstance.IsValid() );
	return *StyleInstance;
}

FName FFactDebuggerStyle::GetStyleSetName()
{
	static FName FactDebuggerStyleName( TEXT( "FactDebuggerStyle" ) );
	return FactDebuggerStyleName;
}

FFactDebuggerStyle::FFactDebuggerStyle() : FSlateStyleSet( GetStyleSetName() )
{
	FSlateStyleSet::SetContentRoot( IPluginManager::Get().FindPlugin( TEXT( "SimpleFactsManager" ) )->GetBaseDir() / TEXT( "Resources" ) );

	// This block exists purely because of the brush "ListView.PinnedItemShadow".
	// This brush is used in SListRow::SListViewPinnedRowWidget::Construct as a shadow to indicate parent/child relationship.
	// The problem is, that this brush is set in FStarshipEditorStyle, which is obviously only initialized in Editor.
	// Code below was added to make mentioned brush work in Non-editor builds also
#if WITH_EDITOR == 0
	SetParentStyleName( FAppStyle::GetAppStyleSetName() );
	FAppStyle::SetAppStyleSetName( GetStyleSetName() );

	Set( "ListView.PinnedItemShadow", new IMAGE_BRUSH( "Icons/PinnedItemShadow", FVector2D( 16.f, 8.f ) ) );
#endif
	
	Set( "ClassIcon.FactPreset", new IMAGE_BRUSH_SVG( TEXT( "Icons/FactPreset" ), CoreStyleConstants::Icon16x16 ) );
	Set( "ClassThumbnail.FactPreset", new IMAGE_BRUSH_SVG( TEXT( "Icons/FactPreset" ), CoreStyleConstants::Icon64x64 ) );
	Set( "Icons.LeafFacts", new IMAGE_BRUSH_SVG( "Icons/LeafFacts", CoreStyleConstants::Icon16x16 ) );
	Set( "Icons.DefinedFacts", new IMAGE_BRUSH_SVG( "Icons/DefinedFacts", CoreStyleConstants::Icon16x16 ) );
	Set( "Icons.Star.Outline", new IMAGE_BRUSH_SVG( "Icons/StarOutline", CoreStyleConstants::Icon16x16 ) );
	Set( "Icons.Star.OutlineFilled", new IMAGE_BRUSH_SVG( "Icons/StarOutlineFilled", CoreStyleConstants::Icon16x16 ) );
	Set( "Icons.Reset", new IMAGE_BRUSH_SVG( "Icons/Reset", CoreStyleConstants::Icon16x16 ) );
	
	Set( "RichText.StarOutline", FInlineTextImageStyle()
			.SetImage( IMAGE_BRUSH_SVG( "Icons/StarOutline", CoreStyleConstants::Icon16x16 ) )
			.SetBaseline( 0 )
		);

	Set( "Colors.FactChanged", FLinearColor{ 0.1f, 0.5f, 0.1f, 0.2f } );
	Set( "Colors.FactPreset", FColor( 243, 113, 42 ) );

	Set( "NameFont", DEFAULT_FONT( "Regular", 9 ) );
	Set( "PathFont", DEFAULT_FONT( "Light", 10 ) );
	
}
