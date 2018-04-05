// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class FluidSurfaceEngine : ModuleRules
{
	public FluidSurfaceEngine(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"FluidSurfaceEngine/Private"
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
				"ShaderCore",
				"RHI"
			}
		);
	}
}