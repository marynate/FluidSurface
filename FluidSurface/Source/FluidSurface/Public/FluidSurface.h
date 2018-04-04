#pragma once

#include "EngineMinimal.h"

#include "ModuleManager.h"

class FFluidSurface : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule( ) override;
	virtual void ShutdownModule( ) override;
};