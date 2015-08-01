
#pragma once

#include "FluidSurfaceOscillator.generated.h"

/** Fluid Surface modifier extension to perform oscillation */
UCLASS( )
class AFluidSurfaceOscillator : public AFluidSurfaceModifier
{
	GENERATED_UCLASS_BODY( )

public:

#if WITH_EDITOR
	virtual void PreEditChange( UProperty* PropertyAboutToChange ) override;
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif
	/* End UObject interface */

public:

	/** Frequency of oscillation */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidOscillator" )
	float Frequency;

	/** Phase */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidOscillator" )
	uint8 Phase;

	/** Strength of the oscillation */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidOscillator" )
	float Strength;

	/** Radius of the oscillation */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidOscillator" )
	float Radius;

#if WITH_EDITORONLY_DATA
	// Reference to the sphere component for radius
	UPROPERTY( )
	class USphereComponent* RadiusComponent;
#endif

private:

	float OscTime;

public:

	/** Update the modifier */
	virtual void Update( float DeltaTime );
};