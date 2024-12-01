// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetDefinition_FactsPreset.h"

#include "ContentBrowserMenuContexts.h"
#include "FactsEditorStyle.h"
#include "FactsPreset.h"
#include "SimpleFactsEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


namespace MenuExtentions_FactsPreset
{
	void OpenPresetInFactsEditor( const FToolMenuContext& MenuContext )
	{
		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			TArray<UFactsPreset*> FactsPresets = Context->LoadSelectedObjects<UFactsPreset>(  );
			check( FactsPresets.Num() == 1 );
			FSimpleFactsEditorModule::Get().LoadPresetIntoEditor( FactsPresets[0] );
		}
	}
	
	bool CanOpenPresetInFactsEditor( const FToolMenuContext& MenuContext )
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
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic( &OpenPresetInFactsEditor );
				UIAction.IsActionVisibleDelegate = FToolMenuCanExecuteAction::CreateStatic( &CanOpenPresetInFactsEditor );
			
				Section.AddMenuEntry(
					"FactsPreset_OpenInFactsEditor",
					LOCTEXT( "FactsPreset_OpenInFactsEditor", "Open this Preset in FactsEditor" ),
					LOCTEXT( "FactsPreset_OpenInFactsEditorTooltip", "Open this Preset in FactsEditor" ),
					FSlateIcon(FFactsEditorStyleStyle::GetStyleSetName(), "ClassIcon.FactsPreset"),
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
	return FColor(243, 113, 42);
}

TSoftClassPtr<UObject> UAssetDefinition_FactsPreset::GetAssetClass() const
{
	return UFactsPreset::StaticClass();
}

#undef LOCTEXT_NAMESPACE