// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Engine/CancellableAsyncAction.h"
#include "AsyncAction_ListenForFactChanges.generated.h"

/**
 * 
 */
UCLASS()
class SIMPLEFACTS_API UAsyncAction_ListenForFactChanges : public UCancellableAsyncAction
{
	GENERATED_BODY()
public:
	/**
	 * Asynchronously waits for a fact change to be broadcast.
	 *
	 * @param FactTag			The Fact tag to listen for
	 */
	UFUNCTION(BlueprintCallable, Category = Messaging, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ListenForFactChanges* ListenForFactChanges( UObject* WorldContextObject, FFactTag Tag );


	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FAsyncFactDelegate, int32, CurrentValue );

	// Executes when value of the fact is changed. If a fact was undefined before changing value, OnFactBecameDefined will be executed first
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Value Changed"))
	FAsyncFactDelegate OnFactValueChanged;

	// Executes when fact state changes from undefined to defined. Executes before OnFactValueChanged
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Became Defined"))
	FAsyncFactDelegate OnFactBecameDefined;

	// // Executes when fact state changes from defined to undefined. OnFactValueChanged will not be executed
	// UPROPERTY(BlueprintAssignable, meta = (DisplayName = "Became Undefined"))
	// FAsyncFactDelegate OnFactBecameUndefined;

private:
	UFUNCTION()
	void HandleFactValueChanged( int32 CurrentValue );

	UFUNCTION()
	void HandleFactBecameDefined( int32 CurrentValue );

private:
	TWeakObjectPtr< UWorld > WorldPtr;
	FFactTag Tag;
};
