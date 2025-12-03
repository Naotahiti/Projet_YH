// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Projet_YH : ModuleRules
{
	public Projet_YH(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","AIModule",
         "GameplayTasks","NavigationSystem",});
	}
}
