
#include "FluidSurfacePrivatePCH.h"

AFluidSurfaceActor::AFluidSurfaceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FluidSurfaceComponent = CreateDefaultSubobject<UFluidSurfaceComponent>(TEXT("FluidSurfaceComponent0"));
	RootComponent = FluidSurfaceComponent;
}