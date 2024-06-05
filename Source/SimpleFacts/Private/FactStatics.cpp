// Fill out your copyright notice in the Description page of Project Settings.


#include "FactStatics.h"

#include "FactLogChannels.h"
#include "FactSubsystem.h"

void UFactStatics::ChangeFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType)
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		FactSubsystem.ChangeFactValue( Tag, NewValue, ChangeType );
		return;
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::ChangeFactValue: WorldContextObject is null" ) );
}

void UFactStatics::ResetFactValue(const UObject* WorldContextObject, const FFactTag Tag)
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		FactSubsystem.ResetFactValue( Tag );
		return;
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::ResetFactValue: WorldContextObject is null" ) );
}

bool UFactStatics::TryGetFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue)
{
	if ( WorldContextObject )
	{
		const UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.TryGetFactValue( Tag, OutValue );
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::ResetFactValue: WorldContextObject is null" ) );
	return false;
}

bool UFactStatics::IsFactDefined(const UObject* WorldContextObject, const FFactTag Tag)
{
	if ( WorldContextObject )
	{
		const UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.IsFactDefined( Tag );
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::IsFactDefined: WorldContextObject is null" ) );
	return false;
}

bool UFactStatics::CheckFactValue(const UObject* WorldContextObject, const FFactTag Tag, int32 WantedValue,
	EFactCompareOperator Operator)
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.CheckFactSimpleCondition( FSimpleFactCondition( Tag, WantedValue, Operator ) );
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::CheckFactValue: WorldContextObject is null" ) );
	return false;
}

bool UFactStatics::CheckFactSimpleCondition(const UObject* WorldContextObject, FSimpleFactCondition Condition)
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.CheckFactSimpleCondition( Condition );
	}

	UE_LOG( LogFact, Error, TEXT( "UFactStatics::CheckFactValue: WorldContextObject is null" ) );
	return false;
}
