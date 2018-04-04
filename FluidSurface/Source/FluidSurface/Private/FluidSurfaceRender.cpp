
#include "FluidSurfaceRender.h"

#include "EngineGlobals.h"
#include "RHI.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "VertexFactory.h"
#include "PackedNormal.h"
#include "LocalVertexFactory.h"
#include "PrimitiveViewRelevance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/CollisionProfile.h"
#include "Curves/CurveFloat.h"
#include "SceneManagement.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "LevelUtils.h"
#include "ShowFlags.h"
#include "Camera/CameraTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture2DDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DynamicMeshBuilder.h"
#include "Components/SceneCaptureComponent2D.h"
#include "StaticMeshResources.h"

#include "FluidSurfaceComponent.h"

/** Render Data */

/** Initialise render resources */
void FFluidSurfaceRenderData::InitResources( UFluidSurfaceComponent* Component )
{
	HasTessellationData = Component->BuildTessellationData;
	NumPrimitives = ( Component->FluidXSize - 1 ) * ( Component->FluidYSize - 1 ) * 2;
	MaxVertexIndex = ( Component->FluidXSize * Component->FluidYSize ) - 1;

	FillVertexBuffer( Component, VertexBuffer.Vertices );
	FillIndexBuffer( Component, IndexBuffer.Indices );

	/* Init vertex factory */
	VertexFactory.Init( &VertexBuffer );

	/* Enqueue initialization of render resources */
	BeginInitResource( &VertexBuffer );
	BeginInitResource( &IndexBuffer );
	BeginInitResource( &VertexFactory );

	/* Create adjacency data */
	if( Component->BuildTessellationData )
	{
		/* Change sizes of quads to match tessellation ratio */
		int32 XSize = (int32) ( Component->FluidXSize * Component->TessellationRatio );
		int32 YSize = (int32) ( Component->FluidYSize * Component->TessellationRatio );
		
		NumPrimitives = ( XSize - 1 ) * ( YSize - 1 ) * 2;
		MaxVertexIndex = ( XSize*YSize ) - 1;

		/* Build adjacency data */
		FillAdjacencyBuffer( Component, AdjacencyIndexBuffer.Indices );
		BeginInitResource( &AdjacencyIndexBuffer );
	}

	/* Initialise fluid texture */
	FRHIResourceCreateInfo CreateInfo;
	FluidTextureResource = RHICreateTexture2D( Component->FluidXSize, Component->FluidYSize, PF_A32B32G32R32F, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo );

	FluidTextureUAV = RHICreateUnorderedAccessView( FluidTextureResource );

	ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER(
		FInitializeFluidVertexBuffers,
		FRWBuffer&, FluidVerts0, FluidVerts0,
		FRWBuffer&, FluidVerts1, FluidVerts1,
		FReadBufferStructured&, FluidPLingBuffer, FluidPLingBuffer,
		UFluidSurfaceComponent*, Component, Component,
		{
			/* Intialise fluid buffers */
			FluidVerts0.Initialize(sizeof(float), Component->FluidXSize * Component->FluidYSize, PF_R32_FLOAT);
			FluidVerts1.Initialize(sizeof(float), Component->FluidXSize * Component->FluidYSize, PF_R32_FLOAT);

			/* Initialise buffer for storing Plings */
			FluidPLingBuffer.Initialize(sizeof(FFluidSurfacePLingParameters), MAX_FLUID_PLINGS, BUF_Volatile);
		});
}

/** Release render resources */
void FFluidSurfaceRenderData::ReleaseResources( )
{
	/* Free render resources */
	ReleaseResourceAndFlush( &VertexBuffer );
	ReleaseResourceAndFlush( &IndexBuffer );
	ReleaseResourceAndFlush( &VertexFactory );

	if( HasTessellationData )
		ReleaseResourceAndFlush( &AdjacencyIndexBuffer );

	FluidTextureUAV.SafeRelease( );
	FluidTextureResource.SafeRelease( );

	FluidVerts0.Release( );
	FluidVerts1.Release( );

	FluidPLingBuffer.Release( );
}

/** Calculates a new origin when using a reduced number of vertices for tessellation */
FVector FFluidSurfaceRenderData::CalcNewOrigin( int32 GridType, int32 XSize, int32 YSize, float GridSpacing )
{
	float RadX;
	float RadY;

	if( GridType == 1 )
	{
		RadX = ( 0.5f * ( XSize - 1 ) * GridSpacing );
		RadY = ROOT3OVER2 * ( 0.5f * ( YSize - 1 ) * GridSpacing );
	}
	else
	{
		RadX = ( 0.5f * ( XSize - 1 ) * GridSpacing );
		RadY = ( 0.5f * ( YSize - 1 ) * GridSpacing );
	}

	return FVector( -RadX, -RadY, 0.f );
}

/** Fill the vertex buffer */
void FFluidSurfaceRenderData::FillVertexBuffer( UFluidSurfaceComponent* Component, TArray<FFluidSurfaceVertex>& OutVertices )
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_FluidSurfaceSceneProxy_FillVertexBuffer );

	FFluidSurfaceVertex Vertex;
	int32 x, y;

	int32 XSize, YSize;
	float GridSpacing;
	FVector Origin;

	/* Adjust sizes for tessellation based on ratio */
	if( HasTessellationData )
	{
		XSize = (int32) ( Component->FluidXSize * Component->TessellationRatio );
		YSize = (int32) ( Component->FluidYSize * Component->TessellationRatio );
		GridSpacing = Component->FluidGridSpacing * ( ( (float) Component->FluidXSize - 1 ) / ( (float) XSize - 1 ) );
		Origin = CalcNewOrigin( Component->FluidGridType, XSize, YSize, GridSpacing );
	}
	else
	{
		XSize = Component->FluidXSize;
		YSize = Component->FluidYSize;
		GridSpacing = Component->FluidGridSpacing;
		Origin = Component->FluidOrigin;
	}

	/* Hex Grid */
	if( Component->FluidGridType == EFluidGridType::FGT_Hexagonal )
	{
		const FVector DefNorm = FVector( 0, 0, 1 );
		const float RecipXSizeMinusHalf = 1.f / ( (float) XSize - 0.5f );

		const float yMin = Origin.Y;
		const float yMax = Origin.Y + ( ROOT3OVER2 * ( YSize - 1 ) * GridSpacing );
		const float xMinEven = Origin.X;
		const float xMaxEven = Origin.X + ( XSize - 1 ) * GridSpacing;
		const float xMinOdd = Origin.X + ( 0.5f * GridSpacing );
		const float xMaxOdd = Origin.X + ( (float) XSize - 0.5f ) * GridSpacing;

		float dY = ROOT3OVER2 * GridSpacing;
		float NormalZ = 4 * dY * GridSpacing;

		// Y == 0 ROW
		for( x = 0; x < XSize; x++ )
		{
			Vertex.Position.X = Origin.X + ( x * GridSpacing );
			Vertex.Position.Y = yMin;
			Vertex.Position.Z = Origin.Z;

			Vertex.Normal = DefNorm;
			Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipXSizeMinusHalf );
			Vertex.TextureCoordinate.Y = 0.0f;

			OutVertices.Add( Vertex );
		}

		// Y == 1 -> FluidYSize-2 ROWS
		for( y = 1; y < YSize - 1; y++ )
		{
			float RowV = ( 1.f * (float) y / ( YSize - 1 ) );
			float RowY = Origin.Y + ( ROOT3OVER2 * y * GridSpacing );

			// ODD ROW
			if( y & 0x01 )
			{
				// X == 0
				Vertex.Position.X = xMinOdd;
				Vertex.Position.Y = RowY;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = ( 1.f * 0.5f * RecipXSizeMinusHalf );
				Vertex.TextureCoordinate.Y = RowV;

				OutVertices.Add( Vertex );

				// X == 1 -> FluidXSize-2
				for( x = 1; x < XSize - 1; x++ )
				{
					Vertex.Position.X = Origin.X + ( ( (float) x + 0.5f ) * GridSpacing );
					Vertex.Position.Y = RowY;
					Vertex.Position.Z = Origin.Z;

					Vertex.Normal = DefNorm;
					Vertex.TextureCoordinate.X = ( 1.f * ( (float) x + 0.5f ) * RecipXSizeMinusHalf );
					Vertex.TextureCoordinate.Y = RowV;

					OutVertices.Add( Vertex );
				}

				// X == FluidXSize - 1
				Vertex.Position.X = xMaxOdd;
				Vertex.Position.Y = RowY;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = 1.f;
				Vertex.TextureCoordinate.Y = RowV;

				OutVertices.Add( Vertex );
			}

			// EVEN ROW
			else
			{
				// X == 0
				Vertex.Position.X = xMinEven;
				Vertex.Position.Y = RowY;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = 0.0f;
				Vertex.TextureCoordinate.Y = RowV;

				OutVertices.Add( Vertex );

				// X == 1 -> FluidXSize-2
				for( x = 1; x < XSize - 1; x++ )
				{
					Vertex.Position.X = Origin.X + ( (float) x * GridSpacing );
					Vertex.Position.Y = RowY;
					Vertex.Position.Z = Origin.Z;

					Vertex.Normal = DefNorm;
					Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipXSizeMinusHalf );
					Vertex.TextureCoordinate.Y = RowV;

					OutVertices.Add( Vertex );
				}

				// X == FluidXSize - 1
				Vertex.Position.X = xMaxEven;
				Vertex.Position.Y = RowY;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = ( 1.f * ( XSize - 1 ) * RecipXSizeMinusHalf );
				Vertex.TextureCoordinate.Y = RowV;

				OutVertices.Add( Vertex );
			}
		}

		// Y == FluidYSize-1 ROW

		// if last row is odd
		if( ( YSize - 1 ) & 0x01 )
		{
			for( x = 0; x < XSize; x++ )
			{
				Vertex.Position.X = Origin.X + ( ( (float) x + 0.5f ) * GridSpacing );
				Vertex.Position.Y = yMax;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = ( 1.f * ( (float) x + 0.5f ) * RecipXSizeMinusHalf );
				Vertex.TextureCoordinate.Y = 1.f;

				OutVertices.Add( Vertex );
			}
		}
		else // if even
		{
			for( x = 0; x < XSize; x++ )
			{
				Vertex.Position.X = Origin.X + ( x * GridSpacing );
				Vertex.Position.Y = yMax;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipXSizeMinusHalf );
				Vertex.TextureCoordinate.Y = 1.f;

				OutVertices.Add( Vertex );
			}
		}
	}
	else
	{
		/* RECT GRID */

		const FVector DefNorm = FVector( 0, 0, 1 );
		const float RecipFluidXSizeMin1 = ( 1.f / ( (float) XSize - 1 ) );
		
		// Y == 0 ROW
		for( x = 0; x < XSize; x++ )
		{
			Vertex.Position.X = Origin.X + ( x * GridSpacing );
			Vertex.Position.Y = Origin.Y;
			Vertex.Position.Z = Origin.Z;

			Vertex.Normal = DefNorm;
			Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipFluidXSizeMin1 );
			Vertex.TextureCoordinate.Y = 0.0f;

			OutVertices.Add( Vertex );
		}

		// Y == 1 -> FluidYSize - 2 ROWS
		for( y = 1; y < YSize - 1; y++ )
		{
			float RowV = ( 1.f * (float) y / ( YSize - 1 ) );
			float RowY = Origin.Y + ( y * GridSpacing );

			Vertex.Position.X = Origin.X;
			Vertex.Position.Y = RowY;
			Vertex.Position.Z = Origin.Z;

			Vertex.Normal = DefNorm;
			Vertex.TextureCoordinate.X = 0.0f;
			Vertex.TextureCoordinate.Y = RowV;

			OutVertices.Add( Vertex );

			// X == 1 -> FluidXSize - 2
			for( x = 1; x < XSize - 1; x++ )
			{
				Vertex.Position.X = Origin.X + ( (float) x * GridSpacing );
				Vertex.Position.Y = RowY;
				Vertex.Position.Z = Origin.Z;

				Vertex.Normal = DefNorm;
				Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipFluidXSizeMin1 );
				Vertex.TextureCoordinate.Y = RowV;

				OutVertices.Add( Vertex );
			}

			// X = FluidXSize - 1
			Vertex.Position.X = Origin.X + ( XSize - 1 ) * GridSpacing;
			Vertex.Position.Y = RowY;
			Vertex.Position.Z = Origin.Z;

			Vertex.Normal = DefNorm;
			Vertex.TextureCoordinate.X = 1.f;
			Vertex.TextureCoordinate.Y = RowV;

			OutVertices.Add( Vertex );
		}

		// Y == FluidYSize - 1 ROW
		for( x = 0; x < XSize; x++ )
		{
			Vertex.Position.X = Origin.X + ( x * GridSpacing );
			Vertex.Position.Y = Origin.Y + ( YSize - 1 ) * GridSpacing;
			Vertex.Position.Z = Origin.Z;

			Vertex.Normal = DefNorm;
			Vertex.TextureCoordinate.X = ( 1.f * (float) x * RecipFluidXSizeMin1 );
			Vertex.TextureCoordinate.Y = 1.f;

			OutVertices.Add( Vertex );
		}
	}
}

/** Fill index buffer with data */
void FFluidSurfaceRenderData::FillIndexBuffer( UFluidSurfaceComponent* Component, TArray<int32>& OutIndices )
{
	int32 x, y;
	int32 XSize, YSize;

	if( HasTessellationData )
	{
		XSize = (int32) ( Component->FluidXSize * Component->TessellationRatio );
		YSize = (int32) ( Component->FluidYSize * Component->TessellationRatio );
	}
	else
	{
		XSize = Component->FluidXSize;
		YSize = Component->FluidYSize;
	}

	if( Component->FluidGridType == EFluidGridType::FGT_Hexagonal )
	{
		// For each row
		for( y = 0; y < YSize - 1; y++ )
		{
			if( y & 0x01 ) // Odd row
			{
				// For each element
				for( x = 0; x < XSize - 1; x++ )
				{
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				}
			}
			else // Even row
			{
				for( x = 0; x < XSize - 1; x++ )
				{
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
				}
			}
		}
	}
	else
	{
		for( y = 0; y < YSize - 1; y++ )
		{
			for( x = 0; x < XSize - 1; x++ )
			{
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
			}
		}
	}
}

/** Fill adjacency buffer with data */
void FFluidSurfaceRenderData::FillAdjacencyBuffer( UFluidSurfaceComponent* Component, TArray<int32>& OutIndices )
{
	int32 XSize, YSize;

	XSize = (int32) ( Component->FluidXSize * Component->TessellationRatio );
	YSize = (int32) ( Component->FluidYSize * Component->TessellationRatio );

	if( Component->FluidGridType == EFluidGridType::FGT_Hexagonal )
	{
		// For each row
		for( int y = 0; y < YSize - 1; y++ )
		{
			if( y & 0x01 ) // Odd row
			{
				// For each element
				for( int x = 0; x < XSize - 1; x++ )
				{
					/* Face A */
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					/* Face B */
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				}
			}
			else // Even row
			{
				for( int x = 0; x < XSize - 1; x++ )
				{
					/* Face A */
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					/* Face B */
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );

					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );

					OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
					OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
					OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
				}
			}
		}
	}
	else
	{
		for( int y = 0; y < YSize - 1; y++ )
		{
			for( int x = 0; x < XSize - 1; x++ )
			{
				/* Face A */
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 1 ) );

				/* Face B */
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );

				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );

				OutIndices.Add( ( y + 0 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 0 ) );
				OutIndices.Add( ( y + 1 ) * XSize + ( x + 1 ) );
			}
		}
	}
}

/** Scene Proxy */

FFluidSurfaceSceneProxy::FFluidSurfaceSceneProxy( UFluidSurfaceComponent* Component )
	: FPrimitiveSceneProxy( Component )
	, Material( NULL )
	, DynamicData( NULL )
	, BodySetup( Component->BodySetup )
	, RenderData( Component->RenderData.Get( ) )
	, MaterialRelevance( Component->GetMaterialRelevance( GetScene( ).GetFeatureLevel( ) ) )
	, Time( 0.0f )
	, LevelColor( 1,1,1 )
	, PropertyColor( 1,1,1 )
	, WireframeColor( Component->GetWireframeColor( ) )
	, CollisionResponse( Component->GetCollisionResponseToChannels( ) )
{
	check( RenderData );

	Material = Component->FluidMaterial;
	if( Material == NULL )
	{
		Material = UMaterial::GetDefaultMaterial( MD_Surface );
	}

}

FFluidSurfaceSceneProxy::~FFluidSurfaceSceneProxy( )
{
	if( DynamicData != NULL ) { delete DynamicData; }
}

SIZE_T FFluidSurfaceSceneProxy::GetTypeHash( ) const
{
	static size_t UniquePointer;
	return reinterpret_cast< size_t >( &UniquePointer );
}

/** Returns the view revelance */
FPrimitiveViewRelevance FFluidSurfaceSceneProxy::GetViewRelevance( const FSceneView* View )
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown( View );
	Result.bShadowRelevance = IsShadowCast( View );
	Result.bDynamicRelevance = true;
	MaterialRelevance.SetPrimitiveViewRelevance( Result );

	return Result;
}

/** Memory footprint */
uint32 FFluidSurfaceSceneProxy::GetMemoryFootprint( ) const
{
	return ( sizeof( *this ) + GetAllocatedSize( ) );
}

/** Size */
uint32 FFluidSurfaceSceneProxy::GetAllocatedSize( ) const
{
	return ( FPrimitiveSceneProxy::GetAllocatedSize( ) );
}

/** Is the viewport currently in a collision view */
bool FFluidSurfaceSceneProxy::IsCollisionView(const FEngineShowFlags& EngineShowFlags, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const
{
	bDrawSimpleCollision = false;
	bDrawComplexCollision = false;

	// If in a 'collision view' and collision is enabled
	const bool bInCollisionView = EngineShowFlags.CollisionVisibility || EngineShowFlags.CollisionPawn;
	if (bInCollisionView && IsCollisionEnabled())
	{
		// See if we have a response to the interested channel
		bool bHasResponse = EngineShowFlags.CollisionPawn && (CollisionResponse.GetResponse(ECC_Pawn) != ECR_Ignore);
		bHasResponse |= EngineShowFlags.CollisionVisibility && (CollisionResponse.GetResponse(ECC_Visibility) != ECR_Ignore);

		if (bHasResponse)
		{
			bDrawComplexCollision = EngineShowFlags.CollisionVisibility;
			bDrawSimpleCollision = EngineShowFlags.CollisionPawn;
		}
	}

	return bInCollisionView;
}

/** Draw mesh */

void FFluidSurfaceSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FluidSurfaceSceneProxy_GetDynamicMeshElements);

	const FEngineShowFlags& EngineShowFlags = ViewFamily.EngineShowFlags;

	bool bDrawSimpleCollision = false;
	bool bDrawComplexCollision = false;
	const bool bInCollisionView = IsCollisionView(EngineShowFlags, bDrawSimpleCollision, bDrawComplexCollision);
	const bool bDrawWireframeCollision = EngineShowFlags.Collision && IsCollisionEnabled();

	bDrawComplexCollision = bInCollisionView && bDrawComplexCollision && AllowDebugViewmodes();

	const bool bDrawMesh = (bInCollisionView) ? (bDrawComplexCollision) : true;
	if (bDrawMesh)
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				const FSceneView* View = Views[ViewIndex];
				GetDynamicMeshElementsForView(View, ViewIndex, Collector, bDrawComplexCollision);
			}
		}
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			if ((bDrawSimpleCollision || bDrawWireframeCollision) && AllowDebugViewmodes())
			{
				const FSceneView* View = Views[ViewIndex];
				const bool bDrawSolid = !bDrawWireframeCollision;
				DebugDrawSimpleCollision(View, ViewIndex, Collector, bDrawSolid);
			}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			// Render bounds
			RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
#endif
		}
	}
}

void FFluidSurfaceSceneProxy::GetDynamicMeshElementsForView(const FSceneView* View, int32 ViewIndex, FMeshElementCollector& Collector, bool bDrawComplexCollision) const
{
	FMaterialRenderProxy* MaterialProxy = Material->GetRenderProxy(IsSelected());

	FMeshBatch& Mesh = Collector.AllocateMesh();
	Mesh.bCanApplyViewModeOverrides = true;
	Mesh.bWireframe = false;
	Mesh.bUseWireframeSelectionColoring = IsSelected();

	const bool bIsWireframeView = AllowDebugViewmodes() && View->Family->EngineShowFlags.Wireframe;

	if (bIsWireframeView)
	{
		auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
			FLinearColor(0, 0.5f, 1.f)
			);

		Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

		MaterialProxy = WireframeMaterialInstance;

		Mesh.bWireframe = true;
		Mesh.bCanApplyViewModeOverrides = false;
	}
	else {
		if (bDrawComplexCollision)
		{
			auto CollisionMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(IsSelected(), IsHovered()),
				WireframeColor
				);
			Collector.RegisterOneFrameMaterialProxy(CollisionMaterialInstance);
			MaterialProxy = CollisionMaterialInstance;
		}
	}

	/* Build render mesh data */
	FMeshBatchElement& BatchElement = Mesh.Elements[0];
	FFluidSurfaceBatchElementParams* BatchElementParams = new FFluidSurfaceBatchElementParams;
	BatchElementParams->FluidTextureResource = RenderData->FluidTextureResource;
	BatchElement.UserData = BatchElementParams;
	BatchElement.IndexBuffer = &RenderData->IndexBuffer;
	BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, false);
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = RenderData->NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = RenderData->MaxVertexIndex;

	Mesh.MaterialRenderProxy = MaterialProxy;
	Mesh.VertexFactory = &RenderData->VertexFactory;
	Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
	Mesh.Type = PT_TriangleList;
	Mesh.DepthPriorityGroup = SDPG_World;

	/* Check to see if adjacency data is required */
	const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation(Material, RenderData->VertexFactory.GetType(), GetScene().GetFeatureLevel());
	
	/* Needs adjacency data */
	if (bRequiresAdjacencyInformation && RenderData->HasTessellationData)
	{
		/* Use adjacency index buffer instead */
		Mesh.Type = PT_12_ControlPointPatchList;
		BatchElement.IndexBuffer = &RenderData->AdjacencyIndexBuffer;
		BatchElement.FirstIndex *= 4;
	}

	// Draw the mesh.
	Collector.AddMesh(ViewIndex, Mesh);
}

void FFluidSurfaceSceneProxy::DebugDrawSimpleCollision(const FSceneView* View, int32 ViewIndex, FMeshElementCollector& Collector, bool bDrawSolid) const
{
	if (!BodySetup)
		return;

	if (FMath::Abs(GetLocalToWorld().Determinant()) >= SMALL_NUMBER)
		return;

	FTransform GeomTransform(GetLocalToWorld());

	if (bDrawSolid)
	{
		auto SolidMaterialInstance = new FColoredMaterialRenderProxy(
			GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(IsSelected(), IsHovered()),
			WireframeColor
			);

		Collector.RegisterOneFrameMaterialProxy(SolidMaterialInstance);

		BodySetup->AggGeom.GetAggGeom(GeomTransform, WireframeColor.ToFColor(true), SolidMaterialInstance, false, true, UseEditorDepthTest(), ViewIndex, Collector);
	}
	else
	{
		const FColor CollisionColor = FColor(157, 149, 223, 255);
		BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(CollisionColor, IsSelected(), IsHovered()).ToFColor(true), nullptr, false, false, UseEditorDepthTest(), ViewIndex, Collector);
	}
}

/** Prepares and executes compute shader */
void FFluidSurfaceSceneProxy::ExecComputeShader( )
{
	if( !DynamicData )
		return;

	/* Get global RHI command list */
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList( );

	/* For now this will be created per frame. Need to figure out where it can sit */
	FFluidSurfaceParameters Parameters;

	/* Populate data */
	Parameters.FluidGridType = (int32) DynamicData->FluidGridType;
	Parameters.FluidGridSpacing = DynamicData->FluidGridSpacing;
	Parameters.FluidXSize = DynamicData->FluidXSize;
	Parameters.FluidYSize = DynamicData->FluidYSize;

	FFluidSurfaceFrameParameters FrameParameters;

	/* Populate per frame data */
	FrameParameters.UpdateRate = DynamicData->UpdateRate;
	FrameParameters.FluidNoiseStrengthMin = DynamicData->FluidNoiseStrength.MinValue;
	FrameParameters.FluidNoiseStrengthMax = DynamicData->FluidNoiseStrength.MaxValue;
	FrameParameters.DeltaTime = DynamicData->DeltaTime;
	FrameParameters.LatestVerts = DynamicData->LatestVerts;
	FrameParameters.FluidOriginDamping = FVector4( DynamicData->FluidOrigin.X, DynamicData->FluidOrigin.Y, DynamicData->FluidOrigin.Z, DynamicData->FluidDamping );
	FrameParameters.Time = Time;
	FrameParameters.FluidNoiseFrequency = DynamicData->FluidNoiseFrequency;
	FrameParameters.FluidSpeed = DynamicData->FluidSpeed;
	FrameParameters.NumPLings = DynamicData->NumPLings;

	/* Populate pling buffer */
	{
		void* Buffer = RHILockStructuredBuffer( RenderData->FluidPLingBuffer.Buffer, 0, DynamicData->NumPLings * sizeof( FFluidSurfacePLingParameters ), RLM_WriteOnly );
		FMemory::Memcpy( Buffer, DynamicData->PLingBuffer, DynamicData->NumPLings * sizeof( FFluidSurfacePLingParameters ) );
		RHIUnlockStructuredBuffer( RenderData->FluidPLingBuffer.Buffer );
	}

	/** Compute shader calculation */
	TShaderMapRef<FFluidSurfaceCS> FluidSurfaceCS( GetGlobalShaderMap( GetScene( ).GetFeatureLevel( ) ) );
	RHICmdList.SetComputeShader( FluidSurfaceCS->GetComputeShader( ) );

	/* Set inputs/outputs and dispatch compute shader */
	FluidSurfaceCS->SetOutput( RHICmdList, RenderData->FluidTextureUAV );
	FluidSurfaceCS->SetUniformBuffers( RHICmdList, Parameters, FrameParameters );
	FluidSurfaceCS->SetParameters( RHICmdList, RenderData->FluidVerts0.UAV, RenderData->FluidVerts1.UAV, RenderData->FluidPLingBuffer.SRV );
	DispatchComputeShader( RHICmdList, *FluidSurfaceCS, DynamicData->FluidXSize / 32, DynamicData->FluidYSize / 32, 1 );
	FluidSurfaceCS->UnbindBuffers( RHICmdList );
}

/** Set dynamic data */
void FFluidSurfaceSceneProxy::SetDynamicData_RenderThread( FFluidSurfaceDynamicData* NewDynamicData )
{
	check( IsInRenderingThread( ) );

	/* Free existing dynamic data */
	if( DynamicData )
	{
		delete DynamicData;
		DynamicData = NULL;
	}

	/* Assign new dynamic data */
	DynamicData = NewDynamicData;

	/* Update time (Used to seed random numbers in compute shader) */
	Time += DynamicData->DeltaTime;
	if( Time > 32767.0f )
		Time = 0.0f;

	/* Execute the compute shader */
	ExecComputeShader( );
}


