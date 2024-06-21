// Fill out your copyright notice in the Description page of Project Settings.

#include "FactTypes.h"

#include "FactSubsystem.h"
#include "Kismet/GameplayStatics.h"

bool FSimpleFactCondition::CheckValue(const UFactSubsystem& FactSubsystem) const
{
	return FactSubsystem.CheckFactSimpleCondition( *this );
}

bool FSimpleFactCondition::IsValid() const
{
	return Tag.IsValid();
}

FString FSimpleFactCondition::ToString() const
{
	return FString::Format( TEXT( "{0} {1} {2}" ), {Tag.ToString()
			, UEnum::GetDisplayValueAsText( Operator ).ToString()
			, FString::FromInt( WantedValue ) } );
}

bool FFactCondition::CheckCondition( const UFactSubsystem& Subsystem ) const
{
	if ( CheckAndDependencies( Subsystem ) == false )
	{
		return false;
	}

	return CheckOrDependencies( Subsystem );
}

bool FFactCondition::IsValid() const
{
	for ( const FSimpleFactCondition& SingleCondition : AndDependencies )
	{
		if ( SingleCondition.IsValid() == false )
		{
			return false;
		}
	}
	
	for ( const FSimpleFactCondition& SingleCondition : OrDependencies )
	{
		if ( SingleCondition.IsValid() == false )
		{
			return false;
		}
	}

	return true;
}

FString FFactCondition::ToString() const
{
	FTextBuilder Builder;
	if ( AndDependencies.Num() > 0 )
	{
		Builder.AppendLine( FString( "And Dependencies:" ) );
		Builder.Indent();
	}

	for ( const FSimpleFactCondition& SingleCondition : AndDependencies )
	{
		if ( SingleCondition.IsValid() )
		{
			Builder.AppendLine( SingleCondition.ToString() );
		}
		else
		{
			Builder.AppendLine( FString( "Fact is not set" ) );
		}
	}
	
	if ( AndDependencies.Num() > 0 )
	{
		Builder.Unindent();
	}

	if ( OrDependencies.Num() > 0 )
	{
		Builder.AppendLine( FString( "Or Dependencies:" ) );
		Builder.Indent();
	}
	for ( const FSimpleFactCondition& SingleCondition : OrDependencies )
	{
		if ( SingleCondition.IsValid() )
		{
			Builder.AppendLine( SingleCondition.ToString() );
		}
		else
		{
			Builder.AppendLine( FString( "Fact is not set" ) );
		}
	}

	if ( OrDependencies.Num() > 0 )
	{
		Builder.Unindent();
	}

	return Builder.ToText().ToString();
}

bool FFactCondition::CheckAndDependencies(const UFactSubsystem& Subsystem) const
{
	// no failure conditions => immediate success
	if ( AndDependencies.Num() == 0 )
	{
		return true;
	}
	
	for ( const FSimpleFactCondition& SingleCondition : AndDependencies )
	{
		const bool bResult = SingleCondition.CheckValue( Subsystem );
		if ( bResult == false )
		{
			// early return if any of the AND conditions is false
			return false;
		}
	}

	return true;
}

bool FFactCondition::CheckOrDependencies(const UFactSubsystem& Subsystem) const
{
	// no failure conditions => immediate success
	if ( OrDependencies.Num() == 0 )
	{
		return true;
	}	
	
	for ( const FSimpleFactCondition& SingleCondition : OrDependencies )
	{
		const bool bResult = SingleCondition.CheckValue( Subsystem );
		if ( bResult )
		{
			// early return if any of the OR conditions is true
			return true;
		}
	}
	
	return false;
}