
#pragma once

#include "FluidSurfaceComponent.generated.h"

#define ROOT3OVER2			(0.866025f)
#define FLUIDBOXHEIGHT		(5)

UENUM( )
namespace EFluidGridType
{
	enum Type
	{
		FGT_Square		= 0,
		FGT_Hexagonal	= 1,
		FGT_MAX			= 2,
	};
}

/** Stores a range of values */
USTRUCT( BlueprintType )
struct FRangedValues
{
	GENERATED_USTRUCT_BODY( );

	/** Minimum value of range */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Range" )
		float MinValue;

	/** Maximum value of range */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Range" )
		float MaxValue;

	/** Get a random number from within the range */
	float GetRand( )
	{
		return FMath::FRandRange( MinValue, MaxValue );
	}
};

UCLASS( ClassGroup = ( Rendering, Common ), HideCategories = ( Object, LOD, Physics, Activation, "Components|Activation" ), EditInLineNew, Meta = ( BlueprintSpawnableComponent ), ClassGroup = Rendering )
class UFluidSurfaceComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY( )

	/* Begin UPrimitiveComponent interface */
	virtual FPrimitiveSceneProxy* CreateSceneProxy( ) override;
	virtual class UBodySetup* GetBodySetup( ) override;
	virtual void ReceiveComponentDamage( float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser ) override;
	virtual void CreatePhysicsState( ) override;
	/* End UPrimitiveComponent interface*/

	/* Begin UMeshComponent interface */
	virtual int32 GetNumMaterials( ) const override;
	virtual UMaterialInterface* GetMaterial( int32 MaterialIndex ) const override;
	/* End UMeshComponent interface*/

	/* Begin USceneComponent interface */
	virtual FBoxSphereBounds CalcBounds( const FTransform& LocalToWorld ) const override;
	/* End USceneComponent interface */

	/* Begin UActorComponent interface */
	virtual void OnRegister( ) override;
	virtual void TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction ) override;
	virtual void SendRenderDynamicData_Concurrent( ) override;
	virtual void CreateRenderState_Concurrent() override;
	/* End UActorComponent interface */

	/* Begin UObject interace */
	virtual void BeginDestroy( ) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif
	/* End UObject interface */

	/* Delegates */
	UFUNCTION( )
	void ComponentTouched( AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );

	/** Color to use when displayed in wireframe */
	FColor GetWireframeColor( ) const;

public:

	/** Grid type */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category="FluidSurface" )
	TEnumAsByte<EFluidGridType::Type> FluidGridType;

	/** Distance between grid points */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FluidSurface" )
	float FluidGridSpacing;

	/** Num vertices in the X direction */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FluidSurface", Meta = ( ClampMin = "32", UIMin = "32", UIMax = "4096" ) )
	int32 FluidXSize;

	/** Num vertices in the Y direction */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FluidSurface", Meta = ( ClampMin = "32", UIMin = "32", UIMax = "4096" ) )
	int32 FluidYSize;

	/** Vertical scale factor */
	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "FluidSurface" )
	float FluidHeightScale;

	/** Wave speed */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float FluidSpeed;

	/** Time scale */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float FluidTimeScale;

	/** Fluid damping - Between 0 and 1 */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface", Meta = ( ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0" ) )
	float FluidDamping;

	/** Fluid Noise Frequency */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float FluidNoiseFrequency;

	/** Fluid Noise Strength */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	FRangedValues FluidNoiseStrength;

	/** Enables ripple testing */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "RippleTest" )
	bool TestRipple;

	/** Speed of test ripple */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "RippleTest", Meta = ( EditCondition = "TestRipple" ) )
	float TestRippleSpeed;

	/** Strength of test ripple */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "RippleTest", Meta = ( EditCondition = "TestRipple" ) )
	float TestRippleStrength;

	/** Radius of test ripple */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "RippleTest", Meta = ( EditCondition = "TestRipple" ) )
	float TestRippleRadius;

	/** Shoot strength */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float ShootStrength;

	/** Shoot radius */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float ShootRadius;

	/** Touch strength */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float TouchStrength;

	/** Ripple velocity factor */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	float RippleVelocityFactor;

	/** Effect to spawn when fluid is shot */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	class UParticleSystem* ShootEffect;

	/** Orient shoot effect */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	bool OrientShootEffect;

	/** Effect to spawn when fluid is touched */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	class UParticleSystem* TouchEffect;

	/** Orient touch effect */
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "FluidSurface" )
	bool OrientTouchEffect;

	/** Fluid color */
	UPROPERTY( )
	FColor FluidColor;

	/** Controls wether or not to tick this component (Editor only) */
	UPROPERTY( EditAnywhere, Category = "FluidSurface" )
	bool UpdateComponent;

	/** Wether or not to build tessellation data */
	UPROPERTY( EditAnywhere, Category = "FluidSurface" )
	bool BuildTessellationData;

	/** The ratio of vertices to use during tessellation */
	UPROPERTY( EditAnywhere, Category = "FluidSurface", Meta = ( EditCondition = "BuildTessellationData", ClampMin = "0.0", UIMin  = "0.0", UIMax = "2.0" ) )
	float TessellationRatio;

	/* List of modifiers that apply to this surface */
	UPROPERTY( )
	TArray<class AFluidSurfaceModifier*> Modifiers;

	/** Collision */
	UPROPERTY( Transient, DuplicateTransient )
	class UBodySetup* BodySetup;

	UPROPERTY( EditAnywhere, Category = "FluidSurface" )
	class UMaterialInterface* FluidMaterial;

	/** Buffer of Plings */
	TArray<struct FFluidSurfacePLingParameters> PLingBuffer;

	/** The number of Plings in the PLingBuffer */
	int NumPLing;

	/** Render data */
	TScopedPointer<class FFluidSurfaceRenderData> RenderData;

	/** Bounding box of the fluid surface */
	FBox FluidBoundingBox;

	/** Rate to update simulation */
	float UpdateRate;

	/** Which buffer to use */
	uint32 LatestVerts;

	/** Origin of fluid surface */
	FVector FluidOrigin;

	float LastDeltaTime;
	float TestRippleAng;

public:

	/** Pling */
	UFUNCTION( BlueprintCallable, Category = "FluidSurface" )
	void Pling( const FVector& Position, float Strength, float Radius );

private:

	/* Perform initialization */
	void Init( );

	/* Get nearest index */
	void GetNearestIndex( const FVector& Pos, int& xIndex, int& yIndex );

	/* Update the physics body */
	void UpdateBody( );

	/* Get World to Component transform */
	FORCEINLINE FTransform GetWorldToComponent( ) const { return FTransform( ComponentToWorld.ToMatrixWithScale( ).Inverse( ) ); }

	/* Create the dynamic data that is sent to the render thread */
	struct FFluidSurfaceDynamicData* CreateDynamicData( );

	friend class FFluidSurfaceSceneProxy;
};