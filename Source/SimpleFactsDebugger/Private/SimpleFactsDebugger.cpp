#include "SimpleFactsDebugger.h"

#include "FactDebuggerStyle.h"
#include "FactStatics.h"
#include "FactSubsystem.h"
#include "SFactDebugger.h"

#if WITH_EDITOR
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#endif

static const FName FactDebuggerTabName( "FactDebugger" );

static FAutoConsoleCommandWithWorld GShowFactDebugger
(
	TEXT( "FactDebugger" ),
	TEXT( "Displays the Fact Debugger" ),
	FConsoleCommandWithWorldDelegate::CreateLambda( []( UWorld* World )
	{
		FGlobalTabmanager::Get()->TryInvokeTab( FactDebuggerTabName );
	} )
);


#define LOCTEXT_NAMESPACE "FSimpleFactsDebuggerModule"
void FSimpleFactsDebuggerModule::StartupModule()
{
	FFactDebuggerStyle::Register();
	
	// Facts Debugger
	FTabSpawnerEntry& Tab = FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FactDebuggerTabName, FOnSpawnTab::CreateRaw( this, &FSimpleFactsDebuggerModule::SpawnFactDebuggerTab ) )
		.SetDisplayName( LOCTEXT( "FactDebugger_Title", "Fact Debugger" ) )
		.SetTooltipText( LOCTEXT( "FactDebugger_ToolTip", "Open Fact Debugger tab." ) )
		.SetIcon( FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "ClassIcon.FactPreset" ) );
	
#if WITH_EDITOR
	Tab.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() );
#endif
}

void FSimpleFactsDebuggerModule::ShutdownModule()
{
	FFactDebuggerStyle::Unregister();
	
	if ( FSlateApplication::IsInitialized() )
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( FactDebuggerTabName );

		if ( FactDebuggerTab.IsValid() )
		{
			FactDebuggerTab.Pin()->RequestCloseTab();
		}
	}
}

// todo: change implementation to allow loading presets from C++/BP
void FSimpleFactsDebuggerModule::LoadFactPreset( const UFactPreset* InPreset )
{
	if ( WeakGameInstance.IsValid() )
	{
		UFactStatics::LoadFactPreset( WeakGameInstance.Pin()->GetWorld(), InPreset );
	}
}

void FSimpleFactsDebuggerModule::LoadFactPresets( const TArray< UFactPreset* >& InPresets )
{
	if ( WeakGameInstance.IsValid() )
	{
		UFactStatics::LoadFactPresets( WeakGameInstance.Pin()->GetWorld(), InPresets );
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
	(void)OnGameInstanceEnded.ExecuteIfBound();
}

bool FSimpleFactsDebuggerModule::IsGameInstanceStarted() const
{
	return WeakGameInstance.IsValid();
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

TSharedRef< SDockTab > FSimpleFactsDebuggerModule::SpawnFactDebuggerTab( const FSpawnTabArgs& SpawnTabArgs )
{
	return SAssignNew( FactDebuggerTab, SDockTab )
		.TabRole( ETabRole::NomadTab )
		[
			SummonFactDebuggerUI().ToSharedRef()
		];
}

TSharedPtr< SWidget > FSimpleFactsDebuggerModule::SummonFactDebuggerUI()
{
	if( IsInGameThread() )
	{
		return SAssignNew( FactDebugger, SFactDebugger )
			.bIsGameStarted( WeakGameInstance.IsValid() );
	}
	
	return {};
}


#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE( FSimpleFactsDebuggerModule, SimpleFactsDebugger )