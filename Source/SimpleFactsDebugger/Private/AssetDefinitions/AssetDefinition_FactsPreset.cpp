// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetDefinition_FactsPreset.h"

#include "ContentBrowserMenuContexts.h"
#include "FactsDebuggerStyle.h"
#include "FactsPreset.h"
#include "SimpleFactsDebugger.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


namespace MenuExtentions_FactsPreset
{
	void OpenPresetInFactsDebugger( const FToolMenuContext& MenuContext )
	{
		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			TArray<UFactsPreset*> FactsPresets = Context->LoadSelectedObjects<UFactsPreset>(  );
			check( FactsPresets.Num() == 1 );
			FSimpleFactsDebuggerModule::Get().LoadPresetIntoDebugger( FactsPresets[0] );
		}
	}
	
	bool CanOpenPresetInFactsDebugger( const FToolMenuContext& MenuContext )
	{
		if ( const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets( MenuContext ) )
		{
			return Context->GetSelectedItems().Num() == 1;
		}

		return false;
	}
	
	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		UToolMenus::RegisterStartupCallback( FSimpleMulticastDelegate::FDelegate::CreateLambda( []
		{
			FToolMenuOwnerScoped OwnerScoped( UE_MODULE_NAME );
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu( UFactsPreset::StaticClass() );

			FToolMenuSection& Section = Menu->FindOrAddSection( "GetAssetActions" );

			{
				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic( &OpenPresetInFactsDebugger );
				UIAction.IsActionVisibleDelegate = FToolMenuCanExecuteAction::CreateStatic( &CanOpenPresetInFactsDebugger );
			
				Section.AddMenuEntry(
					"FactsPreset_OpenInFactsDebugger",
					LOCTEXT( "FactsPreset_OpenInFactsDebugger", "Open this Preset in FactsDebugger" ),
					LOCTEXT( "FactsPreset_OpenInFactsDebuggerTooltip", "Open this Preset in FactsDebugger" ),
					FSlateIcon(FFactsDebuggerStyle::GetStyleSetName(), "ClassIcon.FactsPreset"),
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