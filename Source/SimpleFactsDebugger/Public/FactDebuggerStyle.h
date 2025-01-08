// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

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