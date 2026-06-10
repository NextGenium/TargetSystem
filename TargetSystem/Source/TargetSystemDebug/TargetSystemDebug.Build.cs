// Copyright (c) 2024 NextGenium

using UnrealBuildTool;

public class TargetSystemDebug : ModuleRules
{
	public TargetSystemDebug(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"GameplayDebugger",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"TargetSystem",
		});

		SetupGameplayDebuggerSupport(Target);
	}
}
