
#include "FluidSurfacePrivatePCH.h"
#include "FluidSurfaceRender.h"

#include "Particles/Emitter.h"
#include "Particles/ParticleSystemComponent.h"

const double MyU2Rad = ( double ) 0.000095875262;

inline bool UsingLowDetail( )
{
	// Get detail mode
	static const IConsoleVariable* CVar = IConsoleManager::Get( ).FindConsoleVariable( TEXT( "r.DetailMode" ) );

	// clamp range
	int32 Ret = FMath::Clamp( CVar->GetInt( ), 0, 2 );

	return ( Ret == 0 );
}

UFluidSurfaceComponent::UFluidSurfaceComponent( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;

	FluidGridType = EFluidGridType::FGT_Square;
	FluidGridSpacing = 24.f;
	FluidXSize = 64;
	FluidYSize = 64;
	FluidHeightScale = 1.f;

	FluidSpeed = 170.f;
	FluidTimeScale = 1.f;
	FluidDamping = 0.5f;

	TestRipple = false;
	TestRippleSpeed = 3000.f;
	TestRippleStrength = -20.f;
	TestRippleRadius = 48.f;

	RippleVelocityFactor = -0.05f;

	UpdateRate = 50.f;
	
	ShootStrength = -50.f;
	ShootRadius = 0.f;
	TouchStrength = -50.f;

	FluidNoiseFrequency = 60.f;
	FluidNoiseStrength.MinValue = -70.f;
	FluidNoiseStrength.MaxValue = 70.f;

	BuildTessellationData = false;
	TessellationRatio = 0.125f;
	UpdateComponent = true;
	
	/* Create dynamic delegate for overlapped event */
	OnComponentBeginOverlap.AddDynamic( this, &UFluidSurfaceComponent::ComponentTouched );
}

#if WITH_EDITOR
/** Post edit change property */
void UFluidSurfaceComponent::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty( PropertyChangedEvent );

	//if( PropertyChangedEvent.Property )
	//{
	//	const FName PropertyName = ( PropertyChangedEvent.Property != NULL ) ? PropertyChangedEvent.Property->GetFName( ) : NAME_None;

	//	/* Update the physics body if any properties involving grid shape are changed */
	//	if( PropertyName == GET_MEMBER_NAME_CHECKED( UFluidSurfaceComponent, FluidGridType )
	//		|| PropertyName == GET_MEMBER_NAME_CHECKED( UFluidSurfaceComponent, FluidGridSpacing )
	//		|| PropertyName == GET_MEMBER_NAME_CHECKED( UFluidSurfaceComponent, FluidXSize )
	//		|| PropertyName == GET_MEMBER_NAME_CHECKED( UFluidSurfaceComponent, FluidYSize )
	//		|| PropertyName == GET_MEMBER_NAME_CHECKED( UFluidSurfaceComponent, BuildTessellationData ) )
	//	{
	//		if( RenderData )
	//		{
	//			/* Update render data */
	//			RenderData->ReleaseResources( );
	//			RenderData->InitResources( this );
	//		}

	//		/* Update physics */
	//		UpdateBody( );
	//	}
	//}
}
#endif

/** Init */
void UFluidSurfaceComponent::Init( )
{
	/* Ensure sizes are within limits (In case changed via code, bypassing UI limits) */
	if( FluidXSize < 0 )
		FluidXSize = 0;
	else if( FluidXSize > 4096 )
		FluidXSize = 4096;
	else if( ( FluidXSize % 32 ) != 0 )
		FluidXSize -= ( FluidXSize % 32 );

	if( FluidYSize < 0 )
		FluidYSize = 0;
	else if( FluidYSize > 4096 )
		FluidYSize = 4096;
	else if( ( FluidYSize % 32 ) != 0 )
		FluidYSize -= ( FluidYSize % 32 );

	LatestVerts = 0;

	/* Calculate 'origin' aka. bottom left (min) corner */
	float RadX;
	float RadY;

	if( FluidGridType == EFluidGridType::FGT_Hexagonal )
	{
		RadX = ( 0.5f * ( FluidXSize - 1 ) * FluidGridSpacing );
		RadY = ROOT3OVER2 * ( 0.5f * ( FluidYSize - 1 ) * FluidGridSpacing );
	}
	else
	{
		RadX = ( 0.5f * ( FluidXSize - 1 ) * FluidGridSpacing );
		RadY = ( 0.5f * ( FluidYSize - 1 ) * FluidGridSpacing );
	}

	FluidOrigin.X = -RadX;
	FluidOrigin.Y = -RadY;
	FluidOrigin.Z = 0.f;

	/* Calc bounding box */
	FVector p;
	FluidBoundingBox.Init( );

	p = FluidOrigin;
	p.Z = -FLUIDBOXHEIGHT;
	FluidBoundingBox += p;

	p.X = RadX;
	p.Y = RadY;
	p.Z = FLUIDBOXHEIGHT;
	FluidBoundingBox += p;

	PLingBuffer.Init( MAX_FLUID_PLINGS );
	NumPLing = 0;

	if( RenderData )
	{
		RenderData->ReleaseResources( );
		RenderData = NULL;
	}
	
	/* Create render data */
	RenderData = new FFluidSurfaceRenderData( );
	RenderData->InitResources( this );
}

/** Pling */
void UFluidSurfaceComponent::Pling( const FVector& Position, float Strength, float Radius )
{
	int HitX, HitY;
	GetNearestIndex( Position, HitX, HitY );

	PLingBuffer[ NumPLing ].LocalHitPosition = GetWorldToComponent( ).TransformPosition( Position );
	PLingBuffer[ NumPLing ].HitX = HitX;
	PLingBuffer[ NumPLing ].HitY = HitY;
	PLingBuffer[ NumPLing ].Strength = Strength;
	PLingBuffer[ NumPLing ].Radius = Radius;

	NumPLing++;
}

/** Get nearest index */
void UFluidSurfaceComponent::GetNearestIndex( const FVector& Pos, int& xIndex, int& yIndex )
{
	FVector LocalPos = GetWorldToComponent( ).TransformPosition( Pos );

	xIndex = FMath::RoundToInt( ( LocalPos.X - FluidOrigin.X ) / FluidGridSpacing );
	xIndex = FMath::Clamp( xIndex, 0, FluidXSize - 1 );

	if( FluidGridType == EFluidGridType::FGT_Hexagonal )
		yIndex = FMath::RoundToInt( ( LocalPos.Y - FluidOrigin.Y ) / ( ROOT3OVER2 * FluidGridSpacing ) );
	else
		yIndex = FMath::RoundToInt( ( LocalPos.Y - FluidOrigin.Y ) / FluidGridSpacing );

	yIndex = FMath::Clamp( yIndex, 0, FluidYSize - 1 );
}

/** Scene Proxy creation */
FPrimitiveSceneProxy* UFluidSurfaceComponent::CreateSceneProxy( )
{
	return ( RenderData ) ? new FFluidSurfaceSceneProxy( this ) : NULL;
}

/** Calc Bounds */
FBoxSphereBounds UFluidSurfaceComponent::CalcBounds( const FTransform& LocalToWorld ) const
{
	FBoxSphereBounds NewBounds;
	/*if( BodySetup )
	{
	FBox AggGeomBox = BodySetup->AggGeom.CalcAABB( LocalToWorld );
	if( AggGeomBox.IsValid )
	{
	NewBounds = FBoxSphereBounds( AggGeomBox );
	}
	}
	else*/
	{
		FBox NewBox = FBox( FVector( -( ( FluidXSize * FluidGridSpacing ) / 2.0f ), -( ( FluidYSize * FluidGridSpacing ) / 2.0f ), -10.0f ), FVector( ( FluidXSize * FluidGridSpacing ) / 2.0f, ( FluidYSize * FluidGridSpacing ) / 2.0f, 10.0f ) );
		NewBounds = FBoxSphereBounds( NewBox );
		NewBounds.Origin = GetComponentLocation( );
	}

	return NewBounds;
}

/** On Register */
void UFluidSurfaceComponent::OnRegister( )
{
	Super::OnRegister( );
	Init( );
}

/** Tick */
void UFluidSurfaceComponent::TickComponent( float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
	LastDeltaTime = DeltaTime;

	float SimStep = 1.f / UpdateRate;
	static float Time = 0.0f;

#if WITH_EDITOR

	/* Only update if checked */
	if( !UpdateComponent )
		return;

#endif
	
	/* If this water hasn't been rendered for a while, stop updating */
	if( GetWorld( )->TimeSeconds - LastRenderTime > 1 )
		return;

	Time += DeltaTime;
	if( Time > SimStep )
	{
		Time = 0.0f;
		LatestVerts = !LatestVerts;

		/* Add ripples for actors in the water */
		TArray<struct FOverlapResult> OverlappingActors;

		FCollisionShape CollisionShape;
		CollisionShape.SetBox( FluidBoundingBox.GetExtent( ) );

		/* Find overlapping actors */
		GetWorld( )->OverlapMulti( OverlappingActors, GetComponentLocation( ), GetComponentQuat( ), ECC_WorldDynamic, CollisionShape, FCollisionQueryParams( false ) );

		// @todo: handle better

		/* Iterate through found overlapping actors */
		for( int i = 0; i < OverlappingActors.Num( ); i++ )
		{
			TWeakObjectPtr<AActor> Actor = OverlappingActors[ i ].Actor;

			/* Dont care about self and modifiers */
			if( Actor != NULL && !Actor->IsA( AFluidSurfaceActor::StaticClass( ) ) && !Actor->IsA( AFluidSurfaceModifier::StaticClass( ) ) )
			{
				FVector LocalVel = GetWorldToComponent( ).TransformVector( Actor->GetVelocity( ) );
				float HorizVelMag = LocalVel.Size( );

				Pling( Actor->GetActorLocation( ), RippleVelocityFactor * HorizVelMag, Actor->GetSimpleCollisionRadius( ) );
			}
		}

		/* Do test ripple (moving around in a circle) */
		if( GIsEditor && TestRipple )
		{
			TestRippleAng += SimStep * MyU2Rad * TestRippleSpeed;
			FVector WorldRipplePos, LocalRipplePos;

			float RippleRadius = 0.3f * ( FluidXSize - 1 ) * FluidGridSpacing;
			if( FluidGridType == EFluidGridType::FGT_Hexagonal )
				RippleRadius = FMath::Max( RippleRadius, 0.3f * ( FluidYSize - 1 ) * FluidGridSpacing * ROOT3OVER2 );
			else
				RippleRadius = FMath::Max( RippleRadius, 0.3f * ( FluidYSize - 1 ) * FluidGridSpacing );

			LocalRipplePos.X = ( RippleRadius * FMath::Sin( TestRippleAng ) );
			LocalRipplePos.Y = ( RippleRadius * FMath::Cos( TestRippleAng ) );
			LocalRipplePos.Z = 0.f;

			WorldRipplePos = ComponentToWorld.TransformPosition( LocalRipplePos );
			Pling( WorldRipplePos, TestRippleStrength, TestRippleRadius );
		}

		/* Add modifier effects */
		for( int i = 0; i < Modifiers.Num( ); i++ )
		{
			if( Modifiers[ i ] && Modifiers[ i ]->Active )
				Modifiers[ i ]->Update( DeltaTime );
		}

		/* Need to send new dynamic data */
		MarkRenderDynamicDataDirty( );
	}
}

/** Called when this component receives damage */
void UFluidSurfaceComponent::ReceiveComponentDamage( float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser )
{
	Super::ReceiveComponentDamage( DamageAmount, DamageEvent, EventInstigator, DamageCauser );

	FVector HitLocation;
	if( DamageEvent.IsOfType( FPointDamageEvent::ClassID ) )
	{
		FPointDamageEvent const* const PointDamageEvent = (FPointDamageEvent*) ( &DamageEvent );
		Pling( PointDamageEvent->HitInfo.ImpactPoint, ShootStrength, ShootRadius );

		HitLocation = PointDamageEvent->HitInfo.ImpactPoint;
	}

	if( ShootEffect )
	{
		/* Spawn shoot effect emitter */
		FRotator Rotation = FRotator( 0, 0, 0 );
		AEmitter* Emitter = GetWorld( )->SpawnActor<AEmitter>( HitLocation, Rotation );
		Emitter->ParticleSystemComponent->SetTemplate( ShootEffect );
	}
}

/** Called when an object has overlapped this component */
void UFluidSurfaceComponent::ComponentTouched( AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	if( !Other )
		return;

	FVector ActorLocation = Other->GetActorLocation( );

	if( TouchEffect )
	{
		/* Spawn touch effect emitter */
		FRotator Rotation = FRotator( 0, 0, 0 );
		AEmitter* Emitter = GetWorld( )->SpawnActor<AEmitter>( ActorLocation, Rotation );
		Emitter->ParticleSystemComponent->SetTemplate( TouchEffect );
	}
}

/** Recreate the physics body */
void UFluidSurfaceComponent::UpdateBody( )
{
	if( BodySetup == NULL )
	{
		/* Create physics body */
		BodySetup = ConstructObject<UBodySetup>( UBodySetup::StaticClass( ), this );
	}

	BodySetup->AggGeom.EmptyElements( );
	
	FVector BoxExtents = FluidBoundingBox.GetExtent( );
	FKBoxElem& BoxElem = *new ( BodySetup->AggGeom.BoxElems ) FKBoxElem( BoxExtents.X * 2, BoxExtents.Y * 2, BoxExtents.Z * 2 );
	BoxElem.Center = FVector::ZeroVector;
	BoxElem.Orientation = GetComponentQuat( );
	
	BodySetup->ClearPhysicsMeshes( );
	BodySetup->CreatePhysicsMeshes( );

	/* Setup collision defaults */
	BodyInstance.SetResponseToAllChannels( ECR_Overlap );
	BodyInstance.SetResponseToChannel( ECC_Camera, ECR_Ignore );
	BodyInstance.SetResponseToChannel( ECC_Visibility, ECR_Block );
	BodyInstance.SetInstanceNotifyRBCollision( true );
}

/** Create the dynamic data to send to the render thread */
FFluidSurfaceDynamicData* UFluidSurfaceComponent::CreateDynamicData( )
{
	/* Allocate dynamic data */
	FFluidSurfaceDynamicData* DynamicData = new FFluidSurfaceDynamicData;

	/* Fill dynamic data */
	DynamicData->FluidGridType = FluidGridType;
	DynamicData->FluidXSize = FluidXSize;
	DynamicData->FluidYSize = FluidYSize;
	DynamicData->FluidOrigin = FluidOrigin;
	DynamicData->FluidHeightScale = FluidHeightScale;
	DynamicData->FluidGridSpacing = FluidGridSpacing;
	DynamicData->UpdateRate = UpdateRate;
	DynamicData->FluidNoiseStrength = FluidNoiseStrength;
	DynamicData->DeltaTime = 1.f / UpdateRate;
	DynamicData->FluidDamping = FluidDamping;
	DynamicData->LatestVerts = LatestVerts;
	DynamicData->FluidNoiseFrequency = FluidNoiseFrequency;
	DynamicData->FluidSpeed = FluidSpeed;
	DynamicData->NumPLings = NumPLing;

	for( int i = 0; i < NumPLing; i++ )
		DynamicData->PLingBuffer[ i ] = PLingBuffer[ i ];

	NumPLing = 0;

	return DynamicData;
}

/** Send dynamic data to render thread */
void UFluidSurfaceComponent::SendRenderDynamicData_Concurrent( )
{
	if( SceneProxy )
	{
		FFluidSurfaceDynamicData* DynamicData = CreateDynamicData( );

		/* Enqueue command to send to render thread */
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSendFluidSurfaceDynamicData,
			FFluidSurfaceSceneProxy*, FluidSurfaceSceneProxy, (FFluidSurfaceSceneProxy*) SceneProxy,
			FFluidSurfaceDynamicData*, DynamicData, DynamicData,
			{
				FluidSurfaceSceneProxy->SetDynamicData_RenderThread( DynamicData );
			} );
	}
}

/** Color to use in wireframe mode */
FColor UFluidSurfaceComponent::GetWireframeColor( ) const
{
	return FColor( 0, 255, 255, 255 );
}

/** Begin destroying component */
void UFluidSurfaceComponent::BeginDestroy( )
{
	Super::BeginDestroy( );
	if( RenderData )
		RenderData->ReleaseResources( );
}

void UFluidSurfaceComponent::CreatePhysicsState( )
{
#if WITH_EDITOR
	if( bPhysicsStateCreated )
		DestroyPhysicsState( );

	UpdateBody( );
#endif

	return Super::CreatePhysicsState( );
}

UBodySetup* UFluidSurfaceComponent::GetBodySetup( )
{
	return BodySetup;
}

int32 UFluidSurfaceComponent::GetNumMaterials( ) const
{
	return 1;
}

UMaterialInterface* UFluidSurfaceComponent::GetMaterial( int32 MaterialIndex ) const
{
	return FluidMaterial;
}