// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "FactTypes.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FFactEditorModule"

#if WITH_EDITOR
#include "GameplayTagsEditorModule.h"
#endif

class FSimpleFactsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_EDITOR
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >( TEXT( "PropertyEditor" ) );
		PropertyModule.RegisterCustomPropertyTypeLayout( FFactTag::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic( &FGameplayTagCustomizationPublic::MakeInstance ) );
#endif
	}

	virtual void ShutdownModule() override
	{
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FSimpleFactsModule, SimpleFacts )
// IMPLEMENT_MODULE( FDefaultModuleImpl, SimpleFacts )
