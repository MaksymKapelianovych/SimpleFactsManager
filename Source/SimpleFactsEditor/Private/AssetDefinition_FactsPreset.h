// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AssetDefinitionDefault.h"
#include "AssetDefinition_FactsPreset.generated.h"

UCLASS()
class UAssetDefinition_FactsPreset : public UAssetDefinitionDefault
{
	GENERATED_BODY()

protected:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr< UObject > GetAssetClass() const override;
	// UAssetDefinition End
};
