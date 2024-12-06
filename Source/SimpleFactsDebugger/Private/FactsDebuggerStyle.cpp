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
	const FVector2D Icon16(16.0f, 16.0f);
	const FVector2D Icon64(64.0f, 64.0f);
	
	FSlateStyleSet::SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("SimpleFactsManager"))->GetBaseDir() / TEXT("Resources"));
	Set("ClassIcon.FactsPreset", new IMAGE_BRUSH( TEXT("FactsPreset_16x_white"), Icon16 ));
	Set("ClassThumbnail.FactsPreset", new IMAGE_BRUSH( TEXT("FactsPreset_64x_white"), Icon64 ));
	Set( "Icons.Star.Outline", new IMAGE_BRUSH_SVG( "StarOutline", Icon16 ) );
	Set( "Icons.Star.OutlineFilled", new IMAGE_BRUSH_SVG( "StarOutlineFilled", Icon16 ) );
	
	Set( "RichText.StarOutline", FInlineTextImageStyle()
			.SetImage( IMAGE_BRUSH_SVG( "StarOutline", Icon16 ) )
			.SetBaseline( 0 )
		);
}
