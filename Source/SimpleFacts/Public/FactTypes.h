// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#pragma once
#include "FilteredGameplayTags.h"
#include "FactTypes.generated.h"

class UFactSubsystem;
// Filtered tag that can only accept child tags of "Fact" category
USTRUCT(BlueprintType, Blueprintable, meta = (GameplayTagFilter = "Fact", PresentAsType = "GameplayTag"))
struct SIMPLEFACTS_API FFactTag : public FGameplayTag
{
	GENERATED_BODY()
	END_FILTERED_TAG_DECL( FFactTag, TEXT( "Fact" ) )
};

UENUM(BlueprintType)
enum class EFactCompareOperator : uint8
{
	Equals UMETA(DisplayName = "=="),
	NotEquals UMETA(DisplayName = "!="),
	Greater UMETA(DisplayName = ">"),
	GreaterOrEqual UMETA(DisplayName = ">="),
	Less UMETA(DisplayName = "<"),
	LessOrEqual UMETA(DisplayName = "<="),
	IsUndefined UMETA(DisplayName = "undefined"),
	IsDefined UMETA(DisplayName = "defined")
};

UENUM()
enum class EFactValueChangeType : uint8
{
	Set,
	Add
};

// Helper struct for checking single fact condition
USTRUCT(BlueprintType)
struct SIMPLEFACTS_API FFactCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fact")
	FFactTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fact")
	EFactCompareOperator Operator = EFactCompareOperator::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fact")
	int32 WantedValue = 0;

	FFactCondition() {}

	FFactCondition( FFactTag InTag, int32 InValue, EFactCompareOperator InOperator)
		: Tag( InTag )
		, Operator( InOperator )
		, WantedValue( InValue )
	{	}

	bool IsValid() const;
	FString ToString() const;
	
private:
	
	bool CheckValue( const UFactSubsystem& FactSubsystem ) const;
};