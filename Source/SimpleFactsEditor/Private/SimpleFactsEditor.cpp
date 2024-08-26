#include "SimpleFactsEditor.h"

#include "SFactsEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

static FAutoConsoleCommandWithWorld GShowFactsEditor
(
	TEXT("FactsEditor"),
	TEXT("Displays the Facts editor"),
	FConsoleCommandWithWorldDelegate::CreateLambda( []( UWorld* World )
	{
		FGlobalTabmanager::Get()->TryInvokeTab( FTabId("FactsEditorAppNew") );
	} )
);


#define LOCTEXT_NAMESPACE "FSimpleFactsEditorModule"
void FSimpleFactsEditorModule::StartupModule()
{
	
	
	// // GameplayCue editor
	// FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FName(TEXT("FactsEditorApp")), FOnSpawnTab::CreateRaw(this, &FSimpleFactsEditorModule::SpawnFactsEditorTab))
	// 	.SetDisplayName( LOCTEXT( "FactsEditorTabTitle", "Facts Editor"))
	// 	.SetTooltipText( LOCTEXT( "FactsEditorTooltipText", "Open Facts Editor tab."))
	// 	.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

}

void FSimpleFactsEditorModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( FName(TEXT("FactsEditorApp")) );

		if (FactsEditorTab.IsValid())
		{
			FactsEditorTab.Pin()->RequestCloseTab();
		}
	}
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
		// return SAssignNew(FactsEditor, SFactsEditor, nullptr);
		return SNullWidget::NullWidget;
	}
	
	return {};
}


#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSimpleFactsEditorModule, SimpleFactsEditor)