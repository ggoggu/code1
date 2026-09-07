// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WildCard : ModuleRules
{
	public WildCard(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG",
			"ModelViewViewModel", "GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", 
			"SlateCore" });
		
		PublicIncludePaths.AddRange(new string[]
		{
			"WildCard",
			"WildCard/InGame/Game",
			"WildCard/InGame/Interface",
			"WildCard/InGame/Battle",
			"WildCard/InGame/Data",
			"WildCard/InGame/Unit",
			"WildCard/InGame/Card",
			"WildCard/InGame/UI",
			"WildCard/InGame/Map",
            "WildCard/InGame/Artifact",
			"WildCard/InGame/Effect"
			
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
