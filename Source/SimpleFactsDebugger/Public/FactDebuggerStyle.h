// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct SIMPLEFACTSDEBUGGER_API FFactDebuggerStyle final : public FSlateStyleSet
{
	/** Register style set */
	static void Register();
	/** Unregister style set */
	static void Unregister();

	/** Access the singleton instance for this style set */
	static FFactDebuggerStyle& Get();
	static FName GetStyleSetName();

	FFactDebuggerStyle();
	virtual ~FFactDebuggerStyle() override = default;

private:

	static TSharedPtr< FFactDebuggerStyle > StyleInstance;
};