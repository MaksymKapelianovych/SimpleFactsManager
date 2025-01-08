// Fill out your copyright notice in the Description page of Project Settings.

#include "FactTypes.h"

#include "FactSubsystem.h"
#include "Kismet/GameplayStatics.h"

bool FFactCondition::CheckValue( const UFactSubsystem& FactSubsystem ) const
{
	return FactSubsystem.CheckFactCondition( *this );
}

bool FFactCondition::IsValid() const
{
	return Tag.IsValid();
}

FString FFactCondition::ToString() const
{
	return FString::Format( TEXT( "{0} {1} {2}" ), { Tag.ToString()
			, UEnum::GetDisplayValueAsText( Operator ).ToString()
			, FString::FromInt( WantedValue ) } );
}