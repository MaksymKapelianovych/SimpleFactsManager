// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FFactsEditorStyleStyle final : public FSlateStyleSet
{
	/** Register style set */
	static void Register();
	/** Unregister style set */
	static void Unregister();

	/** Access the singleton instance for this style set */
	static FFactsEditorStyleStyle& Get();
	static FName GetStyleSetName();

	FFactsEditorStyleStyle();
	virtual ~FFactsEditorStyleStyle() override = default;

private:

	static TSharedPtr<FFactsEditorStyleStyle> StyleInstance;
};