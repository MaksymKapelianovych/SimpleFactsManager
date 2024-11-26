// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FactsDebuggerSettingsLocal.generated.h"

USTRUCT()
struct FSearchToggleState
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bIsToggleChecked = true;
	UPROPERTY()
	FText SearchText;
};


UCLASS( Config = EditorPerProjectUserSettings, DefaultConfig )
class SIMPLEFACTSEDITOR_API UFactsDebuggerSettingsLocal : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE(FOnChanged);
	FOnChanged OnChangedDelegate;
	
	UPROPERTY(Config, EditAnywhere)
	bool bShowRootFactTag = true;

	UPROPERTY(Config, EditAnywhere)
	bool bShowFullFactNames = false;

	UPROPERTY(Config, EditAnywhere)
	bool bShouldPinParentRows = true;

	UPROPERTY(Config)
	TArray< FSearchToggleState > ToggleStates;
};
