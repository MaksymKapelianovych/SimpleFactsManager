// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetDefinition_FactsPreset.h"

#include "ContentBrowserMenuContexts.h"
#include "FactsDebuggerStyle.h"
#include "FactsPreset.h"
#include "SimpleFactsDebugger.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


namespace MenuExtentions_FactsPreset
{
	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		UToolMenus::RegisterStartupCallback( FSimpleMulticastDelegate::FDelegate::CreateLambda( []
		{
			FToolMenuOwnerScoped OwnerScoped( UE_MODULE_NAME );
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu( UFactsPreset::StaticClass() );

			FToolMenuSection& Section = Menu->FindOrAddSection( "GetAssetActions" );

			{
				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda( []( const FToolMenuContext& MenuContext )
				{
					if ( const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets( MenuContext ) )
					{
						TArray< UFactsPreset* > Presets = Context->LoadSelectedObjects< UFactsPreset >();
						FSimpleFactsDebuggerModule::Get().LoadFactPresets( Presets );
					}
				} );
				UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda( []( const FToolMenuContext& MenuContext )
				{
					return FSimpleFactsDebuggerModule::Get().IsGameInstanceStarted();
				} );
			
				Section.AddMenuEntry(
					"FactsPreset_Load",
					LOCTEXT( "FactsPreset_Load", "Load preset(s)" ),
					LOCTEXT( "FactsPreset_LoadTooltip", "Load all facts from preset(s) (only in PIE)" ),
					FSlateIcon( FFactsDebuggerStyle::GetStyleSetName(), "ClassIcon.FactsPreset" ),
					UIAction
				);
			}
			
		} ) );
	});
}

FText UAssetDefinition_FactsPreset::GetAssetDisplayName() const
{
	return LOCTEXT( "FactsPreset", "Facts Preset" );
}

FLinearColor UAssetDefinition_FactsPreset::GetAssetColor() const
{
	return FFactsDebuggerStyle::Get().GetColor( "Colors.FactsPreset" );
}

TSoftClassPtr<UObject> UAssetDefinition_FactsPreset::GetAssetClass() const
{
	return UFactsPreset::StaticClass();
}

#undef LOCTEXT_NAMESPACE