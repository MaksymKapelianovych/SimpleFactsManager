#include "SimpleFactsEditor.h"

#include "FactsEditorStyle.h"
#include "FactSubsystem.h"
#include "SFactsEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/SlateStyleRegistry.h"

static FAutoConsoleCommandWithWorld GShowFactsEditor
(
	TEXT("FactsEditor"),
	TEXT("Displays the Facts editor"),
	FConsoleCommandWithWorldDelegate::CreateLambda( []( UWorld* World )
	{
		FGlobalTabmanager::Get()->TryInvokeTab( FTabId("FactsEditorApp") );
	} )
);


#define LOCTEXT_NAMESPACE "FSimpleFactsEditorModule"
void FSimpleFactsEditorModule::StartupModule()
{
	FFactsEditorStyleStyle::Register();
	
	// Facts editor
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FName(TEXT("FactsEditorApp")), FOnSpawnTab::CreateRaw(this, &FSimpleFactsEditorModule::SpawnFactsEditorTab))
		.SetDisplayName( LOCTEXT( "FactsEditorTabTitle", "Facts Editor"))
		.SetTooltipText( LOCTEXT( "FactsEditorTooltipText", "Open Facts Editor tab."))
		.SetIcon( FSlateIcon( FFactsEditorStyleStyle::GetStyleSetName(), "ClassIcon.FactsPreset" ) )
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

}

void FSimpleFactsEditorModule::ShutdownModule()
{
	FFactsEditorStyleStyle::Unregister();
	
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( FName(TEXT("FactsEditorApp")) );

		if (FactsEditorTab.IsValid())
		{
			FactsEditorTab.Pin()->RequestCloseTab();
		}
	}
}

void FSimpleFactsEditorModule::LoadPresetIntoEditor( UFactsPreset* InPresetToLoad )
{
	if ( FactsEditor.IsValid() )
	{
		FactsEditor->LoadFactsPreset( InPresetToLoad );
	}
	else
	{
		if (FGlobalTabmanager::Get()->TryInvokeTab( FTabId("FactsEditorApp") ).IsValid() )
		{
			FactsEditor->LoadFactsPreset( InPresetToLoad );
		}
	}
}

void FSimpleFactsEditorModule::HandleGameInstanceStarted( UGameInstance* GameInstance )
{
	WeakGameInstance = GameInstance;
	(void)OnGameInstanceStarted.ExecuteIfBound();
}

void FSimpleFactsEditorModule::HandleGameInstanceEnded()
{
	WeakGameInstance = nullptr;
}

UFactSubsystem* FSimpleFactsEditorModule::TryGetFactSubsystem() const
{
	if ( WeakGameInstance.IsValid() )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WeakGameInstance->GetWorld() );
		return &FactSubsystem;
	}

	return nullptr;
}

void FSimpleFactsEditorModule::HandlePIEStarted( const bool bIsSimulating )
{
	bPIEActive = true;
}

void FSimpleFactsEditorModule::HandlePIEEnded( const bool bIsSimulating )
{
	bPIEActive = false;
}

TSharedRef<SDockTab> FSimpleFactsEditorModule::SpawnFactsEditorTab( const FSpawnTabArgs& SpawnTabArgs )
{
	return SAssignNew(FactsEditorTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SummonFactsEditorUI().ToSharedRef()
		];
}

TSharedPtr<SWidget> FSimpleFactsEditorModule::SummonFactsEditorUI()
{
	if( IsInGameThread() )
	{
		return SAssignNew(FactsEditor, SFactsEditor);
	}
	
	return {};
}


#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSimpleFactsEditorModule, SimpleFactsEditor)