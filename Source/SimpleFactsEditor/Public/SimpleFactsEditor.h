#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SFactsEditor;

class FSimpleFactsEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedRef<SDockTab> SpawnFactsEditorTab( const FSpawnTabArgs& SpawnTabArgs );
    TSharedPtr<SWidget> SummonFactsEditorUI();
    
    TWeakPtr<SDockTab> FactsEditorTab;
    TSharedPtr<SFactsEditor> FactsEditor;
};
