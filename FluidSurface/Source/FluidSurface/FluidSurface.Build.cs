// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class FluidSurface : ModuleRules
{
	public FluidSurface(ReadOnlyTargetRules Target) : base(Target)
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