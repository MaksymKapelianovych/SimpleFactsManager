#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SFactDebugger;
class UFactPreset;
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

    void LoadFactPreset( const UFactPreset* InPreset );
    void LoadFactPresets( const TArray< UFactPreset*>& InPresets );
    
    bool IsGameInstanceStarted() const;
    
    DECLARE_DELEGATE( FGameInstanceStateChanged )
    FGameInstanceStateChanged OnGameInstanceStarted;
    FGameInstanceStateChanged OnGameInstanceEnded;

    UFactSubsystem* TryGetFactSubsystem() const;

private:
    void HandleGameInstanceStarted( UGameInstance* GameInstance );
    void HandleGameInstanceEnded();

private:
    TSharedRef< SDockTab > SpawnFactDebuggerTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr< SWidget > SummonFactDebuggerUI();
    
    TWeakPtr< SDockTab > FactDebuggerTab;
    TWeakPtr< SFactDebugger > FactDebugger;

    TWeakObjectPtr< UGameInstance > WeakGameInstance;
};
