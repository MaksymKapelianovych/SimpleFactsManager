#pragma once
#include "FilteredGameplayTags.h"
#include "FactTypes.generated.h"

class UFactSubsystem;
// Filtered tag that can only accept child tags of "Fact" category
USTRUCT(BlueprintType, Blueprintable, meta = (GameplayTagFilter = "Fact", PresentAsType = "GameplayTag"))
struct SIMPLEFACTS_API FFactTag : public FGameplayTag
{
	GENERATED_BODY()
	END_FILTERED_TAG_DECL(FFactTag, TEXT("Fact"))
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
struct FSimpleFactCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FFactTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFactCompareOperator Operator = EFactCompareOperator::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WantedValue = 0;

	FSimpleFactCondition() {}

	FSimpleFactCondition( FFactTag InTag, int32 InValue, EFactCompareOperator InOperator)
		: Tag( InTag )
		, Operator( InOperator )
		, WantedValue( InValue )
	{	}

private:
	friend struct FFactCondition;
	
	bool CheckValue(const UFactSubsystem& FactSubsystem) const;
	bool IsValid() const;
	FString ToString() const;
};

/** Represents condition for facts, such as:
 * 
 *  AndDependencies:
 *		"Fact.IsInteracted IsTrue"
 *	OrDependencies
 *		"Fact.InteractionCount == 3"
 *
 *	Condition will evaluate to "true" if all of @see AndDependencies will evaluate to "true" and any oh the @see OrDependencies will evaluate to "true".
 *
 *	For now only used in UFlowNode_FactCondition.
 *	Todo: figure out how to use it in BP and if it is needed in BP,
 *	else remove and use AndDependencies/OrDependencies arrays directly in UFlowNode_FactCondition
 */
USTRUCT(BlueprintType)
struct SIMPLEFACTS_API FFactCondition
{
	GENERATED_BODY()

public:
	bool CheckCondition( const UFactSubsystem& Subsystem ) const;
	bool IsValid() const;
	FString ToString() const;

protected:
	bool CheckAndDependencies(const UFactSubsystem& Subsystem) const;
	bool CheckOrDependencies(const UFactSubsystem& Subsystem) const;
	
public:
	// And fact dependencies for this condition
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray< FSimpleFactCondition > AndDependencies;

	// Or fact dependencies for this condition
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray< FSimpleFactCondition > OrDependencies;
};