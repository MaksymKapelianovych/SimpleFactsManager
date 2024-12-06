// Fill out your copyright notice in the Description page of Project Settings.


#include "FactsDebuggerStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FFactsDebuggerStyle> FFactsDebuggerStyle::StyleInstance;

void FFactsDebuggerStyle::Register()
{
	check(!StyleInstance.IsValid());
	StyleInstance = MakeShared<FFactsDebuggerStyle>();
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FFactsDebuggerStyle::Unregister()
{
	check(StyleInstance.IsValid());
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	StyleInstance.Reset();
}

FFactsDebuggerStyle& FFactsDebuggerStyle::Get()
{
	check( StyleInstance.IsValid() );
	return *StyleInstance;
}

FName FFactsDebuggerStyle::GetStyleSetName()
{
	static FName FactsDebuggerStyleName( TEXT( "FactsDebuggerStyle" ) );
	return FactsDebuggerStyleName;
}

FFactsDebuggerStyle::FFactsDebuggerStyle() : FSlateStyleSet( GetStyleSetName() )
{
	FSlateStyleSet::SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("SimpleFactsManager"))->GetBaseDir() / TEXT("Resources"));
	
	Set("ClassIcon.FactsPreset", new IMAGE_BRUSH( TEXT("FactsPreset_16x_white"), CoreStyleConstants::Icon16x16 ));
	Set("ClassThumbnail.FactsPreset", new IMAGE_BRUSH( TEXT("FactsPreset_64x_white"), CoreStyleConstants::Icon64x64 ));
	Set( "Icons.Star.Outline", new IMAGE_BRUSH_SVG( "StarOutline", CoreStyleConstants::Icon16x16 ) );
	Set( "Icons.Star.OutlineFilled", new IMAGE_BRUSH_SVG( "StarOutlineFilled", CoreStyleConstants::Icon16x16 ) );
	
	Set( "RichText.StarOutline", FInlineTextImageStyle()
			.SetImage( IMAGE_BRUSH_SVG( "StarOutline", CoreStyleConstants::Icon16x16 ) )
			.SetBaseline( 0 )
		);

	Set( "Colors.FactChanged", FLinearColor{ 0.1f, 0.5f, 0.1f, 0.2f } );
	Set( "Colors.FactsPreset", FColor( 243, 113, 42 ) );
	
}
