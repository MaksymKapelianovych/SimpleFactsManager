// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#include "FactRuntimeDebugSubsystem.h"

#include "SimpleFactsDebugger.h"

void UFactRuntimeDebugSubsystem::Initialize( FSubsystemCollectionBase& Collection )
{
	FSimpleFactsDebuggerModule::Get().HandleGameInstanceStarted( GetGameInstance() );
}

void UFactRuntimeDebugSubsystem::Deinitialize()
{
	FSimpleFactsDebuggerModule::Get().HandleGameInstanceEnded( );
}
