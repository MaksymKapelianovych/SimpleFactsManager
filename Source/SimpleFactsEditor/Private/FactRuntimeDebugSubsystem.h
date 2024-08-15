// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactRuntimeDebugSubsystem.generated.h"

class SFactsEditor;

/**
 * 
 */
UCLASS()
class SIMPLEFACTSEDITOR_API UFactRuntimeDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedRef<SDockTab> SpawnFactsEditorTab( const FSpawnTabArgs& SpawnTabArgs );
	TSharedPtr<SWidget> SummonFactsEditorUI();
    
	TWeakPtr<SDockTab> FactsEditorTab;
	TSharedPtr<SFactsEditor> FactsEditor;
};
