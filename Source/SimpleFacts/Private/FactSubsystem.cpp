// Fill out your copyright notice in the Description page of Project Settings.

#include "FactSubsystem.h"
#include "FactLogChannels.h"
#include "FactSave.h"

UFactSubsystem& UFactSubsystem::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject( WorldContextObject, EGetWorldErrorMode::Assert );
	check( World );
	UFactSubsystem* FactSubsystem = UGameInstance::GetSubsystem<UFactSubsystem>( World->GetGameInstance() );
	check( FactSubsystem );
	return *FactSubsystem;
}

void UFactSubsystem::ChangeFactValue(const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType)
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
		return;
	}
	
	auto GetUpdatedValue = [ChangeType, NewValue] (const int32 Value)
	{
		switch ( ChangeType ) {
		case EFactValueChangeType::Set:
			return NewValue;
		case EFactValueChangeType::Add:
			return Value + NewValue;
		default:
			checkf( false, TEXT( "Execution flow should not reach this line. There are some missing cases in switch statement" ) );
			return 0;
		}
	};
	
	if ( int32* CurrentValue = DefinedFacts.Find( Tag ) )
	{
		int32 UpdatedValue = GetUpdatedValue( *CurrentValue );
		if ( *CurrentValue != UpdatedValue )
		{
			*CurrentValue = UpdatedValue;
			BroadcastValueDelegate( Tag, *CurrentValue );
		}
	}
	else
	{
		int32& Value = DefinedFacts.Add( Tag );
		Value = GetUpdatedValue( Value );

		// first broadcast event, that fact became defined
		BroadcastDefinitionDelegate( Tag, Value );
		BroadcastValueDelegate( Tag, Value );
	}
}

void UFactSubsystem::ResetFactValue(const FFactTag Tag)
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
		return;
	}
	
	if ( DefinedFacts.Contains( Tag ) )
	{
		// just re-add fact to map
		int32 NewValue = DefinedFacts.Add( Tag );
		BroadcastValueDelegate( Tag, NewValue );
	}
}

bool UFactSubsystem::GetFactValueIfDefined( const FFactTag Tag, int32& OutValue ) const
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
		return false;
	}
	
	if ( const int32* TagValue = DefinedFacts.Find( Tag ) )
	{
		OutValue = *TagValue;
		return true;
	}
	return false;
}

bool UFactSubsystem::TryGetFactValue(const FFactTag Tag, int32& OutValue) const
{
	return GetFactValueIfDefined( Tag, OutValue );
}

bool UFactSubsystem::CheckFactSimpleCondition(const FSimpleFactCondition& Condition) const
{
	if ( Condition.Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Condition.Tag.ToString() );
		return false;
	}
	
	const int32* FactValue = DefinedFacts.Find( Condition.Tag );

	switch ( Condition.Operator ) {
	case EFactCompareOperator::Equals:
		return FactValue && *FactValue == Condition.WantedValue;
	case EFactCompareOperator::NotEquals:
		return FactValue && *FactValue != Condition.WantedValue;
	case EFactCompareOperator::Greater:
		return FactValue && *FactValue > Condition.WantedValue;
	case EFactCompareOperator::GreaterOrEqual:
		return FactValue && *FactValue >= Condition.WantedValue;
	case EFactCompareOperator::Less:
		return FactValue && *FactValue < Condition.WantedValue;
	case EFactCompareOperator::LessOrEqual:
		return FactValue && *FactValue <= Condition.WantedValue;
	case EFactCompareOperator::IsUndefined:
		return FactValue == nullptr;
	case EFactCompareOperator::IsDefined:
		return FactValue != nullptr;
	default:
		checkf( false, TEXT( "Execution flow should not reach this line. There are some missing cases in switch statement" ) );
	}

	return false;
}

bool UFactSubsystem::CheckFactCondition(const FFactCondition& Condition) const
{
	return Condition.IsValid() && Condition.CheckCondition( *this );
}

bool UFactSubsystem::IsFactDefined(const FFactTag Tag) const
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
		return false;
	}
	
	return DefinedFacts.Contains( Tag );
}

FFactChanged& UFactSubsystem::GetOnFactValueChangedDelegate(FFactTag Tag)
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
	}
	
	return ValueDelegates.FindOrAdd( Tag );
}

FFactChanged& UFactSubsystem::GetOnFactBecameDefinedDelegate(FFactTag Tag)
{
	if ( Tag.IsValid() == false )
	{
		UE_LOG( LogFact, Error, TEXT( "Passed fact tag %s is not valid" ), *Tag.ToString() );
	}
	
	return DefinitionDelegates.FindOrAdd( Tag );
}

void UFactSubsystem::OnGameSaved(UFactSaveGame* SaveGame) const
{
	SaveGame->Facts = DefinedFacts;
}

void UFactSubsystem::OnGameLoaded(const UFactSaveGame* SaveGame)
{
	DefinedFacts = SaveGame->Facts;
}

void UFactSubsystem::BroadcastValueDelegate(const FFactTag Tag, int32 Value)
{
	if ( FFactChanged* Delegate = ValueDelegates.Find( Tag ) )
	{
		Delegate->Broadcast( Value );
	}
}

void UFactSubsystem::BroadcastDefinitionDelegate(const FFactTag Tag, int32 Value)
{
	if ( FFactChanged* Delegate = DefinitionDelegates.Find( Tag ) )
	{
		Delegate->Broadcast( Value );
	}
}

#if !UE_BUILD_SHIPPING
FAutoConsoleCommandWithWorldAndArgs UFactSubsystem::ChangeFactValueCommand
(
	TEXT( "Facts.ChangeValue" ),
	TEXT( "Change value to provided one of a fact by given tag" ),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		// Facts.ChangeValue Fact.Tag Value ChangeType = Set
		if ( Args.Num() < 2 )
		{
			UE_LOG( LogFact, Error, TEXT( "Incorrect number of arguments. Facts.ChangeValue Fact.Tag Value ChangeType = Set" ) );
			return;
		}
		
		if (World)
		{
			UFactSubsystem& FactSubsystem = UFactSubsystem::Get( World);
			
			const FFactTag Tag = FFactTag::TryConvert( FGameplayTag::RequestGameplayTag( FName( Args[0] ) ) );
			if ( Tag.IsValid() == false )
			{
				UE_LOG( LogFact, Error, TEXT( "Incorrect tag: %s" ), *Args[0] );
				return;
			}
			
			const FString& Value = Args[1];

			EFactValueChangeType ValueChangeType = EFactValueChangeType::Set;
			if ( Args.IsValidIndex( 2 ) )
			{
				int64 EnumValue = StaticEnum< EFactValueChangeType >()->GetValueByNameString( Args[2] );
				if ( EnumValue == INDEX_NONE )
				{
					UE_LOG( LogFact, Error, TEXT( "Incorrect EFactValueChangeType value: %s"), *Args[2] );
					return;
				}
			
				ValueChangeType = static_cast< EFactValueChangeType >( EnumValue );	
			}

			int32 ValueFromString = 0;
			LexFromString( ValueFromString, *Value );
	
			FactSubsystem.ChangeFactValue( Tag, ValueFromString, ValueChangeType );
			UE_LOG( LogFact, Log, TEXT("ChangeFactValue succeded" ) );
		}
	})
);

FAutoConsoleCommandWithWorldAndArgs UFactSubsystem::GetFactValueCommand
(
	TEXT( "Facts.GetValue" ),
	TEXT( "Prints value of a fact by given tag" ),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		// Facts.GetValue Fact.Tag
		if ( Args.Num() < 1 )
		{
			UE_LOG( LogFact, Error, TEXT( "Incorrect number of arguments. Facts.GetValue Fact.Tag" ) );
			return;
		}

		if (World)
		{
			UFactSubsystem& FactSubsystem = UFactSubsystem::Get( World);

			const FFactTag Tag = FFactTag::TryConvert( FGameplayTag::RequestGameplayTag( FName( Args[0] ) ) );
			if ( Tag.IsValid() == false )
			{
				UE_LOG( LogFact, Error, TEXT( "Incorrect tag: %s" ), *Tag.ToString() );
				return;
			}

			int32 FactValue = 0;
			if ( FactSubsystem.GetFactValueIfDefined( Tag, FactValue) )
			{
				UE_LOG( LogFact, Log, TEXT( "%s: %d" ), *Tag.ToString(), FactValue );
				return;
			}
			
			UE_LOG( LogFact, Log, TEXT( "Fact %s is undefined" ), *Tag.ToString() );
		}
	})
);

FAutoConsoleCommandWithWorld UFactSubsystem::DumpFactsCommand
(
	TEXT( "Facts.Dump" ),
	TEXT( "Prints values of all defined facts" ),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (World)
		{
			UFactSubsystem& FactSubsystem = UFactSubsystem::Get( World);
			UE_LOG( LogFact, Log, TEXT( "Dumping all defined facts" ) );

			for ( auto& [ Tag, Value] : FactSubsystem.DefinedFacts )
			{
				UE_LOG( LogFact, Log, TEXT( "%s: %d" ), *Tag.ToString(), Value );
			}
		}
	})
);

#endif