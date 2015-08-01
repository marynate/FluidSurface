
#pragma once

#define MAX_FLUID_PLINGS 1024

/** PLing Parameters */
struct FFluidSurfacePLingParameters
{
	FVector LocalHitPosition;
	int HitX;
	int HitY;
	float Strength;
	float Radius;
};

/** Dynamic data */
struct FFluidSurfaceDynamicData
{
	/* Fluid Surface Data */
	EFluidGridType::Type FluidGridType;

	float FluidGridSpacing;
	int32 FluidXSize;
	int32 FluidYSize;
	FVector FluidOrigin;
	float FluidHeightScale;
	float FluidDamping;
	float FluidNoiseFrequency;
	float FluidSpeed;
	float UpdateRate;
	float DeltaTime;
	FRangedValues FluidNoiseStrength;

	uint32 LatestVerts;

	FFluidSurfacePLingParameters PLingBuffer[ MAX_FLUID_PLINGS ];
	int NumPLings;
};

/** Encapsulates a GPU read structured buffer with its SRV. */
struct FReadBufferStructured
{
	FStructuredBufferRHIRef Buffer;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FReadBufferStructured( ) : NumBytes( 0 ) {}

	void Initialize( uint32 BytesPerElement, uint32 NumElements, uint32 AdditionalUsage = 0, bool bUseUavCounter = false, bool bAppendBuffer = false )
	{
		check( GRHIFeatureLevel == ERHIFeatureLevel::SM5 );
		NumBytes = BytesPerElement * NumElements;
		FRHIResourceCreateInfo CreateInfo;
		Buffer = RHICreateStructuredBuffer( BytesPerElement, NumBytes, BUF_ShaderResource | AdditionalUsage, CreateInfo );
		SRV = RHICreateShaderResourceView( Buffer );
	}

	void Release( )
	{
		NumBytes = 0;
		Buffer.SafeRelease( );
		SRV.SafeRelease( );
	}
};

/** Render Data */
class FFluidSurfaceRenderData
{
public:

	/** Default constructor */
	FFluidSurfaceRenderData( );

	/** Initialise required resources */
	void InitResources( UFluidSurfaceComponent* Component );

	/** Release any resources */
	void ReleaseResources( );

	/** Populate vertex buffer */
	void FillVertexBuffer( UFluidSurfaceComponent* Component, TArray<FFluidSurfaceVertex>& OutVertices );

	/** Populate index buffer */
	void FillIndexBuffer( UFluidSurfaceComponent* Component, TArray<int32>& OutIndices );

	/** Populate adjacency buffer */
	void FillAdjacencyBuffer( UFluidSurfaceComponent* Component, TArray<int32>& OutIndices );

private:

	/** Calculate new origin for tessellation */
	FVector CalcNewOrigin( int32 GridType, int32 XSize, int32 YSize, float GridSpacing );

public:

	/** Main Vertex Buffer */
	FFluidSurfaceVertexBuffer VertexBuffer;

	/** Main Index Buffer */
	FFluidSurfaceIndexBuffer IndexBuffer;

	/** Adjacency Index Buffer */
	FFluidSurfaceIndexBuffer AdjacencyIndexBuffer;

	/** Main Vertex Factory*/
	FFluidSurfaceVertexFactory VertexFactory;

	/** Main fluid texture */
	FTexture2DRHIRef FluidTextureResource;

	/** UAV for fluid texture */
	FUnorderedAccessViewRHIRef FluidTextureUAV;

	/** Read/Write Temp Buffer 1 */
	FRWBuffer FluidVerts0;

	/** Read/Write Temp Buffer 2 */
	FRWBuffer FluidVerts1;

	/** PLing Structured Buffer */
	FReadBufferStructured FluidPLingBuffer;

	/** Has adjanceny buffer created */
	bool HasTessellationData;

	uint32 NumPrimitives;
	uint32 MaxVertexIndex;
};

/** Scene Proxy */
class FFluidSurfaceSceneProxy : public FPrimitiveSceneProxy
{
public:

	/** Initialization constructor */
	FFluidSurfaceSceneProxy( UFluidSurfaceComponent* Component );

	/** Virtual destructor */
	virtual ~FFluidSurfaceSceneProxy( );

	/* Begin UPrimitiveSceneProxy interface */
	virtual FPrimitiveViewRelevance GetViewRelevance( const FSceneView* View );
	virtual bool CanBeOccluded( ) const override;
	virtual uint32 GetMemoryFootprint( ) const;
	uint32 GetAllocatedSize( ) const;
	virtual void DrawDynamicElements( FPrimitiveDrawInterface* PDI, const FSceneView* View );
	/* End UPrimitiveSceneProxy interface */

	/* Set data sent from the game thread */
	void SetDynamicData_RenderThread( FFluidSurfaceDynamicData* NewDynamicData );

	/* Execute the compute shader */
	void ExecComputeShader( );

protected:

	bool IsCollisionView( const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision ) const;

public:

	/** Fluid Dynamic data */
	FFluidSurfaceDynamicData* DynamicData;

	/** Render data */
	FFluidSurfaceRenderData* RenderData;

private:

	/** Current material */
	UMaterialInterface* Material;

	/** Material relevance */
	FMaterialRelevance MaterialRelevance;

	/** Physics body */
	UBodySetup* BodySetup;

	/** Mesh data */
	FMeshBatch DynamicMesh;

	/** Used to seed random numbers in compute shader */
	float Time;

	/** Colors */
	FLinearColor LevelColor;
	FLinearColor PropertyColor;
	const FLinearColor WireframeColor;

	/** Collision response of this component */
	FCollisionResponseContainer CollisionResponse;
};
