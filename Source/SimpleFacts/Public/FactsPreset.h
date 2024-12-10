// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Engine/DataAsset.h"
#include "FactsPreset.generated.h"


UCLASS()
class SIMPLEFACTS_API UFactsPreset final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, meta = (ForceInlineRow))
	TMap< FFactTag, int32 > PresetValues;
};
