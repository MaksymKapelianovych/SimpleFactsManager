// Fill out your copyright notice in the Description page of Project Settings.


#include "FactRuntimeDebugSubsystem.h"

#include "SFactsEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FSimpleFactsEditorModule"

void UFactRuntimeDebugSubsystem::Initialize( FSubsystemCollectionBase& Collection )
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FName(TEXT("FactsEditorAppNew")), FOnSpawnTab::CreateUObject(this, &UFactRuntimeDebugSubsystem::SpawnFactsEditorTab))
		.SetDisplayName( LOCTEXT( "FactsEditorTabTitle", "Facts Editor New"))
		.SetTooltipText( LOCTEXT( "FactsEditorTooltipText", "Open Facts Editor tab."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void UFactRuntimeDebugSubsystem::Deinitialize()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( FName(TEXT("FactsEditorAppNew")) );

		if (FactsEditorTab.IsValid())
		{
			FactsEditorTab.Pin()->RequestCloseTab();
		}
	}
}

TSharedRef<SDockTab> UFactRuntimeDebugSubsystem::SpawnFactsEditorTab( const FSpawnTabArgs& SpawnTabArgs )
{
	return SAssignNew(FactsEditorTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
		.OnTabClosed_Lambda( [ this ]( TSharedRef< SDockTab > )
		{
			SearchToggles = FactsEditor->GetSearchToggleStates();
			SaveConfig(  );
			
		} )
		[
			SummonFactsEditorUI().ToSharedRef()
		];
}

TSharedPtr<SWidget> UFactRuntimeDebugSubsystem::SummonFactsEditorUI()
{
	if( IsInGameThread() )
	{
		return SAssignNew(FactsEditor, SFactsEditor, TWeakObjectPtr<UGameInstance>(GetGameInstance()), SearchToggles);
	}
	
	return {};
}


#undef LOCTEXT_NAMESPACE
