#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SFactsDebugger;
class UFactsPreset;
class UFactSubsystem;

class SIMPLEFACTSDEBUGGER_API FSimpleFactsDebuggerModule : public IModuleInterface
{
    friend class UFactRuntimeDebugSubsystem;
    
public:
    static FORCEINLINE FSimpleFactsDebuggerModule& Get()
    {
        static FName SimpleFactsDebuggerModule( "SimpleFactsDebugger" );
        return FModuleManager::LoadModuleChecked< FSimpleFactsDebuggerModule >( SimpleFactsDebuggerModule );
    }
    
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void LoadFactPreset( const UFactsPreset* InPreset );
    void LoadFactPresets( const TArray< UFactsPreset*>& InPresets );
    
    bool IsGameInstanceStarted() const;
    
    DECLARE_DELEGATE( FGameInstanceStateChanged )
    FGameInstanceStateChanged OnGameInstanceStarted;
    FGameInstanceStateChanged OnGameInstanceEnded;

    UFactSubsystem* TryGetFactSubsystem() const;

private:
    void HandleGameInstanceStarted( UGameInstance* GameInstance );
    void HandleGameInstanceEnded();

private:
    TSharedRef< SDockTab > SpawnFactsDebuggerTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr< SWidget > SummonFactsDebuggerUI();
    
    TWeakPtr< SDockTab > FactsDebuggerTab;
    TWeakPtr< SFactsDebugger > FactsDebugger;

    TWeakObjectPtr< UGameInstance > WeakGameInstance;
};
