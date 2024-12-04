// Fill out your copyright notice in the Description page of Project Settings.


#include "FactsEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FFactsEditorStyleStyle> FFactsEditorStyleStyle::StyleInstance;

void FFactsEditorStyleStyle::Register()
{
	check(!StyleInstance.IsValid());
	StyleInstance = MakeShared<FFactsEditorStyleStyle>();
	FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

void FFactsEditorStyleStyle::Unregister()
{
	check(StyleInstance.IsValid());
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	StyleInstance.Reset();
}

FFactsEditorStyleStyle& FFactsEditorStyleStyle::Get()
{
	check( StyleInstance.IsValid() );
	return *StyleInstance;
}

FName FFactsEditorStyleStyle::GetStyleSetName()
{
	static FName FlowEditorStyleName( TEXT( "FactsEditorStyle" ) );
	return FlowEditorStyleName;
}

FFactsEditorStyleStyle::FFactsEditorStyleStyle() : FSlateStyleSet( GetStyleSetName() )
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
