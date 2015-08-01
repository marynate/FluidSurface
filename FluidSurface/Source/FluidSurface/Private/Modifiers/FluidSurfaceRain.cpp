
#include "FluidSurfacePrivatePCH.h"

AFluidSurfaceRain::AFluidSurfaceRain(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Strength.MinValue = -10.f;
	Strength.MaxValue = -50.f;
	Radius.MinValue = 1.f;
	Radius.MaxValue = 10.f;
}

#if WITH_EDITOR
/** Pre Edit Change */
void AFluidSurfaceRain::PreEditChange( UProperty* PropertyAboutToChange )
{
	Super::PreEditChange( PropertyAboutToChange );
}

/** Post Edit Change Property */
void AFluidSurfaceRain::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty( PropertyChangedEvent );
}
#endif

/** Update */
void AFluidSurfaceRain::Update( float DeltaTime )
{
	if( !FluidActor )
		return;

	UFluidSurfaceComponent* Component = FluidActor->FluidSurfaceComponent;
	FVector Origin = Component->GetComponentLocation( );
	FVector Extent = Component->FluidBoundingBox.GetExtent( );

	float MinX, MaxX;
	float MinY, MaxY;

	MinX = Origin.X - Extent.X;
	MaxX = Origin.X + Extent.X;

	MinY = Origin.Y - Extent.Y;
	MaxY = Origin.Y + Extent.Y;

	float RandX = FMath::FRandRange( MinX, MaxX );
	float RandY = FMath::FRandRange( MinY, MaxY );

	Component->Pling( FVector( RandX, RandY, 0.f ), Strength.GetRand( ), Radius.GetRand( ) );
}