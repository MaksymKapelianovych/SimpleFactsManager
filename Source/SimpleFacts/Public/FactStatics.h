// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FactStatics.generated.h"

class UFactPreset;

/**
 * 
 */
UCLASS()
class SIMPLEFACTS_API UFactStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Changes fact value depending on EFactValueChangeType.
	// If fact is undefined, then modification is applied to default type's value (for int32 it is 0)
	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void ChangeFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType );

	/**
	 * Only defined facts can be reset now. Can change it in the future, if there will be some use cases for resetting undefined facts.
	 */
	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void ResetFactValue( const UObject* WorldContextObject, const FFactTag Tag );

	/**
	 * If Fact is not defined, then OutValue is also undefined and should not be used! It is necessary to check if "Is Defined?" is true or false
	 * @return false if fact is undefined or WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject", ReturnDisplayName = "Is Defined?", DeprecatedFunction, DeprecationMessage = "Use GetFactValueIfDefined instead"))
	[[nodiscard, deprecated( "Use GetFactValueIfDefined instead" )]] static bool TryGetFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue );

	/**
	 * If Fact is not defined, then OutValue is also undefined and should not be used!
	 * @return false if fact is undefined or WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = Facts, meta = (WorldContext = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
	[[nodiscard]] static bool GetFactValueIfDefined( const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue );
	
	/**
	 * @return false if WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	[[nodiscard]] static bool CheckFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32 WantedValue, EFactCompareOperator Operator );

	/**
	 * Same as @CheckFactValue, but allows to save condition as variable
	 * @return false if WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	[[nodiscard]] static bool CheckFactCondition( const UObject* WorldContextObject, FFactCondition Condition );
	
	/**
	 * @return false if WorldContextObject is null
	 * More specialized version of CheckFactValue, which only tell if fact is defined or not
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	[[nodiscard]] static bool IsFactDefined( const UObject* WorldContextObject, const FFactTag Tag );

	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void LoadFactPreset( const UObject* WorldContextObject, const UFactPreset* Preset );

	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void LoadFactPresets( const UObject* WorldContextObject, const TArray< UFactPreset* >& Presets );
};
