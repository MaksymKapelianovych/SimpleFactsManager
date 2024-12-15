// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FactStatics.generated.h"

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
	static void ChangeFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType);

	/**
	 * Only defined facts can be reset now. Can change it in the future, if there will be some use cases for resetting undefined facts.
	 */
	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void ResetFactValue(const UObject* WorldContextObject, const FFactTag Tag);

	/**
	 * @return false if fact is undefined or WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject", ReturnDisplayName = "Is Defined?"))
	static bool TryGetFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue);

	/**
	 * @return false if WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static bool CheckFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32 WantedValue, EFactCompareOperator Operator);

	/**
	 * @return false if WorldContextObject is null
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static bool CheckFactSimpleCondition(const UObject* WorldContextObject, FSimpleFactCondition Condition);
	
	
	/**
	 * @return false if WorldContextObject is null
	 * More specialized version of CheckFactValue, which only tell if fact is defined or not
	 */ 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static bool IsFactDefined(const UObject* WorldContextObject, const FFactTag Tag);

	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void LoadFactPreset( const UObject* WorldContextObject, const UFactPreset* Preset );

	UFUNCTION(BlueprintCallable, Category = Facts, meta = (WorldContext = "WorldContextObject"))
	static void LoadFactPresets( const UObject* WorldContextObject, const TArray< UFactPreset* >& Presets );
};
