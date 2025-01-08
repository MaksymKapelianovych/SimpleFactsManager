#include "SimpleFactsDebugger.h"

#include "FactDebuggerStyle.h"
#include "FactLogChannels.h"
#include "FactPreset.h"
#include "FactStatics.h"
#include "FactSubsystem.h"
#include "SFactDebugger.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

#if WITH_EDITOR
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#include "SSettingsEditorCheckoutNotice.h"
#include "Engine/AssetManager.h"
#include "Engine/AssetManagerSettings.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

static const FName FactDebuggerTabName( "FactDebugger" );

static FAutoConsoleCommandWithWorld GShowFactDebugger
(
	TEXT( "Facts.Debugger" ),
	TEXT( "Opens Fact Debugger window" ),
	FConsoleCommandWithWorldDelegate::CreateLambda( []( UWorld* World )
	{
		FGlobalTabmanager::Get()->TryInvokeTab( FactDebuggerTabName );
	} )
);


#define LOCTEXT_NAMESPACE "FactDebugger"

void FSimpleFactsDebuggerModule::StartupModule()
{
	FFactDebuggerStyle::Register();
	
	// Facts Debugger
	FTabSpawnerEntry& Tab = FGlobalTabmanager::Get()->RegisterNomadTabSpawner( FactDebuggerTabName, FOnSpawnTab::CreateRaw( this, &FSimpleFactsDebuggerModule::SpawnFactDebuggerTab ) )
		.SetDisplayName( LOCTEXT( "FactDebugger_Title", "Fact Debugger" ) )
		.SetTooltipText( LOCTEXT( "FactDebugger_ToolTip", "Open Fact Debugger tab." ) )
		.SetIcon( FSlateIcon( FFactDebuggerStyle::GetStyleSetName(), "ClassIcon.FactPreset" ) );
	
#if WITH_EDITOR
	Tab.SetGroup( WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory() );

	// Register to get a warning on startup if settings aren't configured correctly
	UAssetManager::CallOrRegister_OnAssetManagerCreated( FSimpleMulticastDelegate::FDelegate::CreateRaw( this, &FSimpleFactsDebuggerModule::HandleAssetManagerCreated ) );
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

void FSimpleFactsDebuggerModule::LoadFactPreset( const UFactPreset* InPreset ) const
{
	if ( WeakGameInstance.IsValid() )
	{
		UFactStatics::LoadFactPreset( WeakGameInstance.Pin()->GetWorld(), InPreset );
	}
}

void FSimpleFactsDebuggerModule::LoadFactPresets( const TArray< UFactPreset* >& InPresets ) const
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

#if WITH_EDITOR
void FSimpleFactsDebuggerModule::HandleAssetManagerCreated()
{
	// Make sure the game has the appropriate asset manager configuration or we won't be able to load game feature data assets
	FPrimaryAssetId DummyGameFeatureDataAssetId( UFactPreset::StaticClass()->GetFName(), NAME_None );
	FPrimaryAssetRules GameDataRules = UAssetManager::Get().GetPrimaryAssetRules( DummyGameFeatureDataAssetId );
	if ( FApp::HasProjectName() && GameDataRules.IsDefault() )
	{
		FMessageLog( "LoadErrors" ).Error()
			->AddToken( FTextToken::Create( FText::Format( LOCTEXT( "MissingRuleForFactPreset", "Asset Manager settings do not include an entry for assets of type {0}, which is required for FactDebugger's \"Presets\" picker to show assets in non-editor builds.\n"), FText::FromName( UFactPreset::StaticClass()->GetFName() ) ) ) )
			->AddToken( FActionToken::Create( LOCTEXT( "AddRuleForFactPreset", "Add entry to PrimaryAssetTypesToScan?" ), FText(),
				FOnActionTokenExecuted::CreateRaw( this, &FSimpleFactsDebuggerModule::AddDefaultGameDataRule ), true ) );
	}
}

void FSimpleFactsDebuggerModule::AddDefaultGameDataRule()
{
	// Check out the ini or make it writable
	UAssetManagerSettings* Settings = GetMutableDefault<UAssetManagerSettings>();

	const FString& ConfigFileName = Settings->GetDefaultConfigFilename();

	bool bSuccess = false;

	FText NotificationOpText;
	if ( SettingsHelpers::IsCheckedOut( ConfigFileName, true ) == false )
	{
		FText ErrorMessage;
		bSuccess = SettingsHelpers::CheckOutOrAddFile( ConfigFileName, true, IsRunningCommandlet() == false, &ErrorMessage );
		if ( bSuccess )
		{
			NotificationOpText = LOCTEXT( "CheckedOutAssetManagerIni", "Rule for {0} added.\nChecked out {1}" );
		}
		else
		{
			UE_LOG( LogFact, Error, TEXT("%s"), *ErrorMessage.ToString() );
			bSuccess = SettingsHelpers::MakeWritable( ConfigFileName );

			if ( bSuccess )
			{
				NotificationOpText = LOCTEXT( "MadeWritableAssetManagerIni", "Rule for {0} added.\nMade {1} writable (you may need to manually add to revision control)" );
			}
			else
			{
				NotificationOpText = LOCTEXT( "FailedToTouchAssetManagerIni", "No rule for {0} was added.\nFailed to check out {1} or make it writable" );
			}
		}
	}
	else
	{
		NotificationOpText = LOCTEXT( "UpdatedAssetManagerIni", "Rule for {0} added.\nUpdated {1}" );
		bSuccess = true;
	}

	// Add the rule to project settings
	if ( bSuccess )
	{
		FPrimaryAssetTypeInfo NewTypeInfo(
			UFactPreset::StaticClass()->GetFName(),
			UFactPreset::StaticClass(),
			false,
			false,
			{ { "/Game" } },
			{}
		);
		NewTypeInfo.Rules.CookRule = EPrimaryAssetCookRule::DevelopmentAlwaysProductionUnknownCook;

		Settings->Modify( true );

		Settings->PrimaryAssetTypesToScan.Add( NewTypeInfo );

		Settings->PostEditChange();
		Settings->TryUpdateDefaultConfigFile();

		UAssetManager::Get().ReinitializeFromConfig();
	}

	// Show a message that the file was checked out/updated and must be submitted
	FNotificationInfo Info( FText::Format( NotificationOpText,
		FText::FromString( UFactPreset::StaticClass()->GetName() ),
		FText::FromString( FPaths::GetCleanFilename(ConfigFileName) )
	) );
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification( Info );
}
#endif

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