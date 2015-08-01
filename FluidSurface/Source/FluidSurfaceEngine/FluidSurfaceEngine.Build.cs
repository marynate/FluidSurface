// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class FluidSurfaceEngine : ModuleRules
	{
		public FluidSurfaceEngine(TargetInfo Target)
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
}