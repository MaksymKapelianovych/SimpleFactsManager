// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
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
class UFactsDebuggerSettingsLocal : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere)
	bool bShowRootFactTag = true;

	UPROPERTY(Config, EditAnywhere)
	bool bShowFullFactNames = false;

	UPROPERTY(Config, EditAnywhere)
	bool bShouldStackHierarchyHeaders = true;

	UPROPERTY(Config, EditAnywhere)
	bool bRemoveFavoritesFromMainTree = true;

	UPROPERTY(Config, EditAnywhere)
	bool bCountOnlyLeafFacts = false;
	
	UPROPERTY(Config, EditAnywhere)
	TEnumAsByte< EOrientation > Orientation = Orient_Horizontal;

	UPROPERTY(Config)
	TArray< FFactTag > FavoriteFacts;

	UPROPERTY(Config)
	TArray< FSearchToggleState > ToggleStates;
};
