// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "GameFramework/SaveGame.h"
#include "FactSave.generated.h"

UCLASS(BlueprintType)
class SIMPLEFACTS_API UFactSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UFactSaveGame() {}

	UPROPERTY()
	TMap< FFactTag, int32 > Facts;
};
