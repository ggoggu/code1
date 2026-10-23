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
			"ModelViewViewModel", "GameplayTags", "Json", "JsonUtilities"
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
			"WildCard/InGame/UI/Widget",
			"WildCard/InGame/UI/ViewModel",
			"WildCard/InGame/Map",
            "WildCard/InGame/Artifact",
			"WildCard/InGame/Effect",
			"WildCard/Editor",
			"WildCard/Editor/SaveCameraTransform",
			"WildCard/Auth",
			"WildCard/Deck",
			"WildCard/Matchmaking",
			"WildCard/Matchmaking/UI"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"LevelEditor",
				"Blutility",
				"UMGEditor",
				"ModelViewViewModelBlueprint",
				"ModelViewViewModelEditor"
			});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
