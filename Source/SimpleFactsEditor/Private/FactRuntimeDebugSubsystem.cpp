// Fill out your copyright notice in the Description page of Project Settings.


#include "FactRuntimeDebugSubsystem.h"

#include "SFactsEditor.h"
#include "SimpleFactsEditor.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FSimpleFactsEditorModule"

void UFactRuntimeDebugSubsystem::Initialize( FSubsystemCollectionBase& Collection )
{
	FSimpleFactsEditorModule::Get().HandleGameInstanceStarted( GetGameInstance() );
}

void UFactRuntimeDebugSubsystem::Deinitialize()
{
	FSimpleFactsEditorModule::Get().HandleGameInstanceEnded( );
}

#undef LOCTEXT_NAMESPACE
