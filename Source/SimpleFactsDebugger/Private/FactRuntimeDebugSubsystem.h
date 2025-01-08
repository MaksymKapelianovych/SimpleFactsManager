// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactRuntimeDebugSubsystem.generated.h"

UCLASS( NotBlueprintType )
class UFactRuntimeDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	virtual void Initialize( FSubsystemCollectionBase& Collection ) override;
	virtual void Deinitialize() override;
};
