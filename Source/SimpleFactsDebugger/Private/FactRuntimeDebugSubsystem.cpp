// Fill out your copyright notice in the Description page of Project Settings.


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
