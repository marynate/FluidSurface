// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class FluidSurface : ModuleRules
	{
		public FluidSurface(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"FluidSurface/Private",
                    "FluidSurface/Private/Modifiers"
				}
				);

            PublicIncludePaths.AddRange(
                new string[] {
                    "FluidSurfaceRender/Public"
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
                    "RHI",
                    "FluidSurfaceEngine"
				}
				);
		}
	}
}