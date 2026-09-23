// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovementPlayground : ModuleRules
{
	public MovementPlayground(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MovementPlayground",
			"MovementPlayground/Variant_Platforming",
			"MovementPlayground/Variant_Platforming/Animation",
			"MovementPlayground/Variant_Combat",
			"MovementPlayground/Variant_Combat/AI",
			"MovementPlayground/Variant_Combat/Animation",
			"MovementPlayground/Variant_Combat/Gameplay",
			"MovementPlayground/Variant_Combat/Interfaces",
			"MovementPlayground/Variant_Combat/UI",
			"MovementPlayground/Variant_SideScrolling",
			"MovementPlayground/Variant_SideScrolling/AI",
			"MovementPlayground/Variant_SideScrolling/Gameplay",
			"MovementPlayground/Variant_SideScrolling/Interfaces",
			"MovementPlayground/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
