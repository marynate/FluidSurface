
#include "FluidSurfacePrivatePCH.h"

AFluidSurfaceOscillator::AFluidSurfaceOscillator( const class FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
	, Frequency( 1.f )
	, Phase( 0.f )
	, Strength( 10.f )
	, Radius( 0.f )
{
#if WITH_EDITORONLY_DATA
	RadiusComponent = CreateEditorOnlyDefaultSubobject<USphereComponent>( TEXT( "SphereComponent0" ) );

	if( !IsRunningCommandlet( ) )
	{
		if( RadiusComponent != nullptr )
		{
			RadiusComponent->AttachParent = SceneComponent;
			RadiusComponent->SetSphereRadius( Radius );
		}
	}
#endif
}

#if WITH_EDITOR
/** Pre Edit Change */
void AFluidSurfaceOscillator::PreEditChange( UProperty* PropertyAboutToChange )
{
	Super::PreEditChange( PropertyAboutToChange );
}

/** Post Edit Change Property */
void AFluidSurfaceOscillator::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	if( PropertyChangedEvent.Property )
	{
		const FName PropertyName = ( PropertyChangedEvent.Property != NULL ) ? PropertyChangedEvent.Property->GetFName( ) : NAME_None;

		if( PropertyName == GET_MEMBER_NAME_CHECKED( AFluidSurfaceOscillator, Radius ) )
			RadiusComponent->SetSphereRadius( Radius );
	}

	Super::PostEditChangeProperty( PropertyChangedEvent );
}
#endif

/** Update the modifier */
void AFluidSurfaceOscillator::Update( float DeltaTime )
{
	if( !FluidActor )
		return;

	OscTime += DeltaTime;

	// If frequency is zero - just set velocity to strength
	float Period, Amp, CurrPhase;

	if( Frequency > 0.0001f )
	{
		Period = 1.f / Frequency;
		CurrPhase = ( FMath::Fmod( OscTime, Period ) * Frequency ) + (float) Phase / 255.f;
		Amp = Strength * FMath::Sin( CurrPhase * PI * 2 );
	}
	else
		Amp = Strength;

	FluidActor->FluidSurfaceComponent->Pling( GetActorLocation( ), Amp, Radius );
}