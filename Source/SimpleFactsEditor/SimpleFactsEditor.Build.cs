// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

using UnrealBuildTool;

public class SimpleFactsEditor : ModuleRules
{
    public SimpleFactsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "SimpleFacts",
                "SimpleFactsDebugger",
                "SlateCore", 
                "GameplayTags",
                "UnrealEd",
                "ToolMenus",
                "AssetDefinition",
                "ContentBrowser",
            }
        );
    }
}