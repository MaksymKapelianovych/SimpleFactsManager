// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct SIMPLEFACTSDEBUGGER_API FFactsDebuggerStyle final : public FSlateStyleSet
{
	/** Register style set */
	static void Register();
	/** Unregister style set */
	static void Unregister();

	/** Access the singleton instance for this style set */
	static FFactsDebuggerStyle& Get();
	static FName GetStyleSetName();

	FFactsDebuggerStyle();
	virtual ~FFactsDebuggerStyle() override = default;

private:

	static TSharedPtr< FFactsDebuggerStyle > StyleInstance;
};