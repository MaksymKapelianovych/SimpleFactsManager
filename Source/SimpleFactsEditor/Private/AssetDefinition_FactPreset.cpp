// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#include "AssetDefinition_FactPreset.h"

#include "ContentBrowserMenuContexts.h"
#include "FactDebuggerStyle.h"
#include "FactPreset.h"
#include "SimpleFactsDebugger.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


namespace MenuExtentions_FactsPreset
{
	static FDelayedAutoRegisterHelper DelayedAutoRegister( EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		UToolMenus::RegisterStartupCallback( FSimpleMulticastDelegate::FDelegate::CreateLambda( []
		{
			FToolMenuOwnerScoped OwnerScoped( UE_MODULE_NAME );
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu( UFactPreset::StaticClass() );

			FToolMenuSection& Section = Menu->FindOrAddSection( "GetAssetActions" );

			{
				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda( []( const FToolMenuContext& MenuContext )
				{
					if ( const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets( MenuContext ) )
					{
						TArray< UFactPreset* > Presets = Context->LoadSelectedObjects< UFactPreset >();
						FSimpleFactsDebuggerModule::Get().LoadFactPresets( Presets );
					}
				} );
				UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateLambda( []( const FToolMenuContext& MenuContext )
				{
					return FSimpleFactsDebuggerModule::Get().IsGameInstanceStarted();
				} );
			
				Section.AddMenuEntry(
					"FactPreset_Load",
					LOCTEXT( "FactPreset_Load", "Load preset(s)" ),
					LOCTEXT( "FactPreset_LoadTooltip", "Load all facts from preset(s) (only in PIE)" ),
					FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "ClassIcon.FactPreset" ),
					UIAction
				);
			}
			
		} ) );
	} );
}

FText UAssetDefinition_FactPreset::GetAssetDisplayName() const
{
	return LOCTEXT( "FactPreset", "Fact Preset" );
}

FLinearColor UAssetDefinition_FactPreset::GetAssetColor() const
{
	return FFactDebuggerStyle::Get().GetColor( "Colors.FactPreset" );
}

TSoftClassPtr<UObject> UAssetDefinition_FactPreset::GetAssetClass() const
{
	return UFactPreset::StaticClass();
}

#undef LOCTEXT_NAMESPACE