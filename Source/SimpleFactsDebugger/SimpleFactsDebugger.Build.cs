// Copyright 2024, Maksym Kapelianovych. Licensed under MIT license.

using UnrealBuildTool;

public class SimpleFactsDebugger : ModuleRules
{
    public SimpleFactsDebugger(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "SimpleFacts",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "InputCore",
                "GameplayTags",
                "Projects",
                "DeveloperSettings",
                "ToolWidgets",
                "ToolMenus",
            }
        );

        if (Target.bCompileAgainstEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "WorkspaceMenuStructure",
                    "SharedSettingsWidgets",
                }
            );
        }

        RuntimeDependencies.Add( System.IO.Path.Combine( ModuleDirectory, "..", "..", "Resources", "Icons", "*") );
    }
}