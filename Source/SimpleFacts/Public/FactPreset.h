// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Engine/DataAsset.h"
#include "FactPreset.generated.h"


UCLASS()
class SIMPLEFACTS_API UFactPreset final : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Fact", meta = (ForceInlineRow))
	TMap< FFactTag, int32 > PresetValues;
};
