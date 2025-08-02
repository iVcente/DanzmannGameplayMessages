// Copyright (C) 2025 Vicente Danzmann. All Rights Reserved.

using UnrealBuildTool;

public class DanzmannGameplayMessages : ModuleRules
{
	public DanzmannGameplayMessages(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;				

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"GameplayTags"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
			}
		);
	}
}
