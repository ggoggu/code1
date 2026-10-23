using UnrealBuildTool;

public class WildCardEditor : ModuleRules
{
    public WildCardEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "WildCard",
            "UnrealEd",
            "PropertyEditor",
            "Slate",
            "SlateCore"
        });
    }
}
