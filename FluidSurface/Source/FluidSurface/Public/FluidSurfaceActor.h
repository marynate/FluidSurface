
#pragma once

#include "FluidSurfaceActor.generated.h"

UCLASS( HideCategories = ( Input, Replication, Physics ) )
class AFluidSurfaceActor : public AActor
{
	GENERATED_UCLASS_BODY( )

	UPROPERTY( Category = FluidSurface, VisibleAnywhere, BlueprintReadOnly )
	TSubobjectPtr<class UFluidSurfaceComponent> FluidSurfaceComponent;
};