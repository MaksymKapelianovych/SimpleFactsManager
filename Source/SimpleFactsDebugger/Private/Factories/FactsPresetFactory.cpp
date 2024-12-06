// Fill out your copyright notice in the Description page of Project Settings.


#include "FactsPresetFactory.h"
#include "FactsPreset.h"

#define LOCTEXT_NAMESPACE "FactsPresetFactory"

UFactsPresetFactory::UFactsPresetFactory()
{
	SupportedClass = UFactsPreset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UFactsPresetFactory::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn )
{
	return NewObject<UFactsPreset>( InParent, InClass, InName, Flags );
}

bool UFactsPresetFactory::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXT_NAMESPACE