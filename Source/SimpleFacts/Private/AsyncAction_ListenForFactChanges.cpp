// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncAction_ListenForFactChanges.h"

#include "FactSubsystem.h"

UAsyncAction_ListenForFactChanges* UAsyncAction_ListenForFactChanges::ListenForFactChanges( UObject* WorldContextObject, FFactTag Tag )
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject, EGetWorldErrorMode::LogAndReturnNull );
	if ( World == nullptr )
	{
		return nullptr;
	}

	UAsyncAction_ListenForFactChanges* Action = NewObject< UAsyncAction_ListenForFactChanges >();
	Action->WorldPtr = World;
	Action->Tag = Tag;
	Action->RegisterWithGameInstance( World );

	return Action;
}

void UAsyncAction_ListenForFactChanges::Activate()
{
	if ( UWorld* World = WorldPtr.Get() )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( World );
		FactSubsystem.GetOnFactValueChangedDelegate( Tag ).AddUObject( this, &ThisClass::HandleFactValueChanged );
		FactSubsystem.GetOnFactBecameDefinedDelegate( Tag ).AddUObject( this, &ThisClass::HandleFactBecameDefined );
		return;
	}

	SetReadyToDestroy();
}

void UAsyncAction_ListenForFactChanges::SetReadyToDestroy()
{
	if ( UWorld* World = WorldPtr.Get() )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( World );
		FactSubsystem.GetOnFactValueChangedDelegate( Tag ).RemoveAll( this );
		FactSubsystem.GetOnFactBecameDefinedDelegate( Tag ).RemoveAll( this );
	}
	
	Super::SetReadyToDestroy();
}

void UAsyncAction_ListenForFactChanges::HandleFactValueChanged( int32 CurrentValue )
{
	if ( OnFactValueChanged.IsBound() == false && OnFactBecameDefined.IsBound() == false )
	{
		SetReadyToDestroy();
		return;
	}
	
	OnFactValueChanged.Broadcast( CurrentValue );
}

void UAsyncAction_ListenForFactChanges::HandleFactBecameDefined( int32 CurrentValue )
{
	if ( OnFactValueChanged.IsBound() == false && OnFactBecameDefined.IsBound() == false )
	{
		SetReadyToDestroy();
		return;
	}
	
	OnFactBecameDefined.Broadcast( CurrentValue );
}
