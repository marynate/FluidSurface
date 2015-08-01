
#pragma once

#include "FluidSurfaceRain.generated.h"

/** Example extension of the Fluid Surface Modifier, to simulate rain falling */
UCLASS( )
class AFluidSurfaceRain : public AFluidSurfaceModifier
{
	GENERATED_UCLASS_BODY( )

public:

	/* Begin UObject interface */
#if WITH_EDITOR
	virtual void PreEditChange( UProperty* PropertyAboutToChange ) override;
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif
	/* End UObject interface */

public:

	/** Range of strength values to use */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidRain" )
	FRangedValues Strength;

	/** Range of radii to use */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidRain" )
	FRangedValues Radius;

public:

	/** Update the modifier */
	virtual void Update( float DeltaTime ) override;
};