#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SFactsDebugger;
class UFactsPreset;
class UFactSubsystem;

class FSimpleFactsDebuggerModule : public IModuleInterface
{
public:
    static FORCEINLINE FSimpleFactsDebuggerModule& Get()
    {
        static FName SimpleFactsDebuggerModule( "SimpleFactsDebugger" );
        return FModuleManager::LoadModuleChecked<FSimpleFactsDebuggerModule>( SimpleFactsDebuggerModule );
    }
    
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void LoadPresetIntoDebugger( UFactsPreset* InPresetToLoad );

    void HandleGameInstanceStarted( UGameInstance* GameInstance );
    void HandleGameInstanceEnded();
    
    DECLARE_DELEGATE(FGameInstanceStarted)
    FGameInstanceStarted OnGameInstanceStarted;

    UFactSubsystem* TryGetFactSubsystem() const;

private:
    void HandlePIEStarted( const bool bIsSimulating );
    void HandlePIEEnded( const bool bIsSimulating );
    
    TSharedRef<SDockTab> SpawnFactsDebuggerTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr<SWidget> SummonFactsDebuggerUI();
    
    TWeakPtr<SDockTab> FactsDebuggerTab;
    TWeakPtr<SFactsDebugger> FactsDebugger;

    TWeakObjectPtr< UGameInstance > WeakGameInstance;
    bool bPIEActive = false;;
};
