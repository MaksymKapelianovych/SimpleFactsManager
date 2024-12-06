#include "SimpleFactsDebugger.h"

#include "FactsDebuggerStyle.h"
#include "FactSubsystem.h"
#include "SFactsDebugger.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Styling/SlateStyleRegistry.h"

static FAutoConsoleCommandWithWorld GShowFactsDebugger
(
	TEXT("FactsDebugger"),
	TEXT("Displays the Facts Debugger"),
	FConsoleCommandWithWorldDelegate::CreateLambda( []( UWorld* World )
	{
		FGlobalTabmanager::Get()->TryInvokeTab( FTabId("FactsDebuggerApp") );
	} )
);


#define LOCTEXT_NAMESPACE "FSimpleFactsDebuggerModule"
void FSimpleFactsDebuggerModule::StartupModule()
{
	FFactsDebuggerStyle::Register();
	
	// Facts Debugger
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FName(TEXT("FactsDebuggerApp")), FOnSpawnTab::CreateRaw(this, &FSimpleFactsDebuggerModule::SpawnFactsDebuggerTab))
		.SetDisplayName( LOCTEXT( "FactsDebuggerTabTitle", "Facts Debugger"))
		.SetTooltipText( LOCTEXT( "FactsDebuggerTooltipText", "Open Facts Debugger tab."))
		.SetIcon( FSlateIcon( FFactsDebuggerStyle::GetStyleSetName(), "ClassIcon.FactsPreset" ) )
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());

}

void FSimpleFactsDebuggerModule::ShutdownModule()
{
	FFactsDebuggerStyle::Unregister();
	
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( FName(TEXT("FactsDebuggerApp")) );

		if (FactsDebuggerTab.IsValid())
		{
			FactsDebuggerTab.Pin()->RequestCloseTab();
		}
	}
}

// todo: change implementation to allow loading presets from C++/BP
void FSimpleFactsDebuggerModule::LoadPresetIntoDebugger( UFactsPreset* InPresetToLoad )
{
	if ( FactsDebugger.IsValid() )
	{
		FactsDebugger.Pin()->LoadFactsPreset( InPresetToLoad );
	}
	else
	{
		if (FGlobalTabmanager::Get()->TryInvokeTab( FTabId("FactsDebuggerApp") ).IsValid() )
		{
			FactsDebugger.Pin()->LoadFactsPreset( InPresetToLoad );
		}
	}
}

void FSimpleFactsDebuggerModule::HandleGameInstanceStarted( UGameInstance* GameInstance )
{
	WeakGameInstance = GameInstance;
	(void)OnGameInstanceStarted.ExecuteIfBound();
}

void FSimpleFactsDebuggerModule::HandleGameInstanceEnded()
{
	WeakGameInstance = nullptr;
}

UFactSubsystem* FSimpleFactsDebuggerModule::TryGetFactSubsystem() const
{
	if ( WeakGameInstance.IsValid() )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WeakGameInstance->GetWorld() );
		return &FactSubsystem;
	}

	return nullptr;
}

void FSimpleFactsDebuggerModule::HandlePIEStarted( const bool bIsSimulating )
{
	bPIEActive = true;
}

void FSimpleFactsDebuggerModule::HandlePIEEnded( const bool bIsSimulating )
{
	bPIEActive = false;
}

TSharedRef<SDockTab> FSimpleFactsDebuggerModule::SpawnFactsDebuggerTab( const FSpawnTabArgs& SpawnTabArgs )
{
	return SAssignNew( FactsDebuggerTab, SDockTab )
		.TabRole( ETabRole::NomadTab )
		[
			SummonFactsDebuggerUI().ToSharedRef()
		];
}

TSharedPtr<SWidget> FSimpleFactsDebuggerModule::SummonFactsDebuggerUI()
{
	if( IsInGameThread() )
	{
		return SAssignNew( FactsDebugger, SFactsDebugger )
			.bIsGameStarted( WeakGameInstance.IsValid() );
	}
	
	return {};
}


#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSimpleFactsDebuggerModule, SimpleFactsDebugger)