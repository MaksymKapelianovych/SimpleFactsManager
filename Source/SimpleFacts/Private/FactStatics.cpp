// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

#include "FactStatics.h"

#include "FactLogChannels.h"
#include "FactPreset.h"
#include "FactSubsystem.h"

void UFactStatics::ChangeFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32 NewValue, EFactValueChangeType ChangeType )
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		FactSubsystem.ChangeFactValue( Tag, NewValue, ChangeType );
		return;
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
}

void UFactStatics::ResetFactValue( const UObject* WorldContextObject, const FFactTag Tag )
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		FactSubsystem.ResetFactValue( Tag );
		return;
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
}

bool UFactStatics::TryGetFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue )
{
	return  GetFactValueIfDefined( WorldContextObject, Tag, OutValue );
}

bool UFactStatics::GetFactValueIfDefined( const UObject* WorldContextObject, const FFactTag Tag, int32& OutValue )
{
	if ( WorldContextObject )
	{
		const UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.GetFactValueIfDefined( Tag, OutValue );
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
	return false;
}

bool UFactStatics::IsFactDefined( const UObject* WorldContextObject, const FFactTag Tag )
{
	if ( WorldContextObject )
	{
		const UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.IsFactDefined( Tag );
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
	return false;
}

bool UFactStatics::CheckFactValue( const UObject* WorldContextObject, const FFactTag Tag, int32 WantedValue, EFactCompareOperator Operator )
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.CheckFactCondition( FFactCondition( Tag, WantedValue, Operator ) );
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
	return false;
}

bool UFactStatics::CheckFactCondition( const UObject* WorldContextObject, FFactCondition Condition )
{
	if ( WorldContextObject )
	{
		UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
		return FactSubsystem.CheckFactCondition( Condition );
	}

	UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
	return false;
}

void UFactStatics::LoadFactPreset( const UObject* WorldContextObject, const UFactPreset* Preset )
{
#if !UE_BUILD_SHIPPING
	if ( WorldContextObject == nullptr )
	{
		UE_LOG( LogFact, Error, TEXT( "%hs: WorldContextObject is null" ), __FUNCTION__ );
		return;
	}

	if ( Preset == nullptr )
	{
		UE_LOG( LogFact, Error, TEXT( "%hs: Preset is null" ), __FUNCTION__ );
		return;
	}

	UFactSubsystem& FactSubsystem = UFactSubsystem::Get( WorldContextObject );
	for ( auto [ Tag, Value ] : Preset->PresetValues )
	{
		FactSubsystem.ChangeFactValue( Tag, Value, EFactValueChangeType::Set );
	}
#endif
}

void UFactStatics::LoadFactPresets( const UObject* WorldContextObject, const TArray< UFactPreset* >& Presets )
{
#if !UE_BUILD_SHIPPING
	for ( const UFactPreset* Preset : Presets )
	{
		if ( Preset == nullptr )
		{
			UE_LOG( LogFact, Error, TEXT( "%hs: Null preset in TArray" ), __FUNCTION__ );
			continue;
		}
		
		LoadFactPreset( WorldContextObject, Preset );
	}
#endif
}
