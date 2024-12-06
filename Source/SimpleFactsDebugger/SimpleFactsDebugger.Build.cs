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
                "WorkspaceMenuStructure",
                "GameplayTags",
                "UnrealEd",
                "EditorStyle", 
                "AssetTools",
                "ToolMenus",
                "ContentBrowser",
                "AssetDefinition",
                "Projects",
                "DetailCustomizations",
                "DeveloperSettings"
            }
        );
    }
}