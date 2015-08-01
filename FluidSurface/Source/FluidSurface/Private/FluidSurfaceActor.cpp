
#include "FluidSurfacePrivatePCH.h"

AFluidSurfaceActor::AFluidSurfaceActor( const class FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
	FluidSurfaceComponent = PCIP.CreateDefaultSubobject<UFluidSurfaceComponent>( this, TEXT( "FluidSurfaceComponent0" ) );
	RootComponent = FluidSurfaceComponent;
}