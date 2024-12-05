#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SFactsEditor;
class UFactsPreset;
class UFactSubsystem;

class FSimpleFactsEditorModule : public IModuleInterface
{
public:
    static FORCEINLINE FSimpleFactsEditorModule& Get()
    {
        static FName SimpleFactsEditorModule( "SimpleFactsEditor" );
        return FModuleManager::LoadModuleChecked<FSimpleFactsEditorModule>( SimpleFactsEditorModule );
    }
    
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void LoadPresetIntoEditor( UFactsPreset* InPresetToLoad );

    void HandleGameInstanceStarted( UGameInstance* GameInstance );
    void HandleGameInstanceEnded();
    
    DECLARE_DELEGATE(FGameInstanceStarted)
    FGameInstanceStarted OnGameInstanceStarted;

    UFactSubsystem* TryGetFactSubsystem() const;

private:
    void HandlePIEStarted( const bool bIsSimulating );
    void HandlePIEEnded( const bool bIsSimulating );
    
    TSharedRef<SDockTab> SpawnFactsEditorTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr<SWidget> SummonFactsEditorUI();
    
    TWeakPtr<SDockTab> FactsEditorTab;
    TWeakPtr<SFactsEditor> FactsEditor;

    TWeakObjectPtr< UGameInstance > WeakGameInstance;
    bool bPIEActive = false;;
};
