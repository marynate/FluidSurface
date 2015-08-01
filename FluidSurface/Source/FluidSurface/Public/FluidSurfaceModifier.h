
#pragma once

#include "FluidSurfaceModifier.generated.h"

/** Base class for Fluid Surface modifiers */
UCLASS( Abstract, HideCategories = ( Input, Collision, Replication ) )
class AFluidSurfaceModifier : public AActor
{
	GENERATED_UCLASS_BODY( )

public:

	/* Begin UObject interace */
	virtual void BeginDestroy( ) override;
#if WITH_EDITOR
	virtual void PreEditChange( UProperty* PropertyAboutToChange ) override;
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif
	/* End UObject interface */

public:

	/** Fluid Surface to modify */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FluidModifier" )
	AFluidSurfaceActor* FluidActor;

	/** Wether or not the modifier is active */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidModifier" )
	bool Active;

#if WITH_EDITORONLY_DATA
	// Reference to the billboard component
	UPROPERTY( )
	class UBillboardComponent* SpriteComponent;
#endif

	// Blank scene component
	UPROPERTY( )
	class USceneComponent* SceneComponent;

public:

	/** Update the modifier */
	virtual void Update( float DeltaTime ) { /* Do nothing */ }
};