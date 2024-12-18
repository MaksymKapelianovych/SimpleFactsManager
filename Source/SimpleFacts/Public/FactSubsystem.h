// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactTypes.h"
#include "FactSubsystem.generated.h"

class UFactSaveGame;
DECLARE_MULTICAST_DELEGATE_OneParam(FFactChanged, int32)

/**
 * 
 */
UCLASS()
class SIMPLEFACTS_API UFactSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * @return the facts subsystem for the game instance associated with the world of the specified object
	 */
	[[nodiscard]] static UFactSubsystem& Get(const UObject* WorldContextObject);

	// Changes fact value depending on EFactValueChangeType.
	// If fact is undefined, then modification is applied to default type's value (for int32 it is 0)
	void ChangeFactValue(const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType);

	/**
	 * Only defined facts can be reset now. Can change it in the future, if there will be some use cases for resetting undefined facts.
	 */
	void ResetFactValue(const FFactTag Tag);

	/**
	 * If Fact is not defined, then OutValue is also undefined and should not be used!
	 * @return false if fact is undefined
	 */
	[[nodiscard]] bool GetFactValueIfDefined(const FFactTag Tag, int32& OutValue) const;
	/**
	 * If Fact is not defined, then OutValue is also undefined and should not be used! It is necessary to check if return value is true or false
	 * @return false if fact is undefined
	 */
	[[nodiscard, deprecated("Use GetFactValueIfDefined instead")]] bool TryGetFactValue(const FFactTag Tag, int32& OutValue) const;

	/**
	 * Returns true if Condition passes.
	 * If condition explicitly checks for value, returns false if value is different or fact is undefined.
	 */ 
	[[nodiscard]] bool CheckFactSimpleCondition(const FSimpleFactCondition& Condition) const;
	[[nodiscard]] bool CheckFactCondition(const FFactCondition& Condition) const;

	/**
	 * More specialized version of CheckFactSimpleCondition, which only tell if fact is defined or not.
	 */ 
	[[nodiscard]] bool IsFactDefined(const FFactTag Tag) const;
	
	FFactChanged& GetOnFactValueChangedDelegate(FFactTag Tag);
	FFactChanged& GetOnFactBecameDefinedDelegate(FFactTag Tag);

	UFUNCTION(BlueprintCallable, Category = "FactSubsystem")
	void OnGameSaved(UFactSaveGame* SaveGame) const;
	
	UFUNCTION(BlueprintCallable, Category = "FactSubsystem")
	void OnGameLoaded(const UFactSaveGame* SaveGame);

private:
	void BroadcastValueDelegate( const FFactTag Tag, int32 Value );
	void BroadcastDefinitionDelegate( const FFactTag Tag, int32 Value );
	
private:
	UPROPERTY(SaveGame)
	TMap<FFactTag, int32> DefinedFacts;

	// maybe merge them together, some struct?
	// also some form of map compaction
	TMap<FFactTag, FFactChanged> ValueDelegates;
	TMap<FFactTag, FFactChanged> DefinitionDelegates;

#if !UE_BUILD_SHIPPING
	static class FAutoConsoleCommandWithWorldAndArgs ChangeFactValueCommand;
	static class FAutoConsoleCommandWithWorldAndArgs GetFactValueCommand;
	static class FAutoConsoleCommandWithWorld		 DumpFactsCommand;
#endif
};
