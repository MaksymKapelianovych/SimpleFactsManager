#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Engine/GameInstance.h"

class SFactDebugger;
class SDockTab;
class FSpawnTabArgs;
class SWidget;
class UFactPreset;
class UFactSubsystem;
class UGameInstance;

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

    void LoadFactPreset( const UFactPreset* InPreset ) const;
    void LoadFactPresets( const TArray< UFactPreset*>& InPresets ) const;
    
    bool IsGameInstanceStarted() const;
    
    DECLARE_DELEGATE( FGameInstanceStateChanged )
    FGameInstanceStateChanged OnGameInstanceStarted;
    FGameInstanceStateChanged OnGameInstanceEnded;

    UFactSubsystem* TryGetFactSubsystem() const;

private:
    void HandleGameInstanceStarted( UGameInstance* GameInstance );
    void HandleGameInstanceEnded();

#if WITH_EDITOR
    void HandleAssetManagerCreated();
    void AddDefaultGameDataRule();
#endif
    
private:
    TSharedRef< SDockTab > SpawnFactDebuggerTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr< SWidget > SummonFactDebuggerUI();
    
    TWeakPtr< SDockTab > FactDebuggerTab;
    TWeakPtr< SFactDebugger > FactDebugger;

    TWeakObjectPtr< UGameInstance > WeakGameInstance;
};
