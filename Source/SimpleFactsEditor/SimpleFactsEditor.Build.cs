using UnrealBuildTool;

public class SimpleFactsEditor : ModuleRules
{
    public SimpleFactsEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        OptimizeCode = CodeOptimization.Never;
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
                "EditorStyle"
            }
        );
    }
}