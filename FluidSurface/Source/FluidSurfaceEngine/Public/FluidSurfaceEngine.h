
#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

DECLARE_DWORD_COUNTER_STAT_EXTERN( TEXT( "Fluid Surface Tris" ), STAT_FluidSurfaceTriangles, STATGROUP_Engine, FLUIDSURFACEENGINE_API );


/** Vertex */
struct FFluidSurfaceVertex
{
	FFluidSurfaceVertex( ) { Tangent = FVector( 1, 0, 0 ); }
	FFluidSurfaceVertex( const FVector& InPosition ) :
		Position( InPosition ),
		Normal( FVector( 0, 0, 1 ) ),
		TextureCoordinate( FVector2D::ZeroVector ),
		Tangent( FVector( 0, 0, 1 ) )
	{
	}

	FFluidSurfaceVertex( const FVector& InPosition, const FVector& InNormal, const FVector2D& InTexCoords ) :
		Position( InPosition ),
		Normal( InNormal ),
		TextureCoordinate( InTexCoords ),
		Tangent( FVector( 0, 0, 1 ) )
	{
	}

	FVector Position;
	FVector Normal;
	FVector Tangent;
	FVector2D TextureCoordinate;
};

/** Vertex Buffer */
class FFluidSurfaceVertexBuffer : public FVertexBuffer
{
public:
	TArray<FFluidSurfaceVertex> Vertices;

	/** Initialise vertex buffer */
	virtual void InitRHI( )
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer( Vertices.Num( ) * sizeof( FFluidSurfaceVertex ), BUF_Static, CreateInfo );

		void* VertexBufferData = RHILockVertexBuffer( VertexBufferRHI, 0, Vertices.Num( ) * sizeof( FFluidSurfaceVertex ), RLM_WriteOnly );
		FMemory::Memcpy( VertexBufferData, Vertices.GetData( ), Vertices.Num( ) * sizeof( FFluidSurfaceVertex ) );
		RHIUnlockVertexBuffer( VertexBufferRHI );

		Vertices.Empty( );
	}
};

/** Index Buffer */
class FFluidSurfaceIndexBuffer : public FIndexBuffer
{
public:
	TArray<int32> Indices;

	/** Initialise index buffer */
	virtual void InitRHI( )
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer( sizeof( int32 ), Indices.Num( ) * sizeof( int32 ), BUF_Static, CreateInfo );

		void* IndexBufferData = RHILockIndexBuffer( IndexBufferRHI, 0, Indices.Num( ) * sizeof( int32 ), RLM_WriteOnly );
		FMemory::Memcpy( IndexBufferData, Indices.GetData( ), Indices.Num( ) * sizeof( int32 ) );
		RHIUnlockIndexBuffer( IndexBufferRHI );

		Indices.Empty( );
	}
};

/** Vertex Factory Shader Param */
class FFluidSurfaceVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
public:

	/** Bind */
	virtual void Bind( const FShaderParameterMap& ParameterMap ) override;

	/** Serialize */
	virtual void Serialize( FArchive& Ar )
	{
		Ar << FluidTextureParameter
			<< FluidTextureSamplerParameter;
	}

	/** Set Mesh */
	virtual void SetMesh( FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags ) const override;

	/** Get Size */
	virtual uint32 GetSize( ) const
	{
		return sizeof( *this );
	}

protected:

	/** Main fluid texture and sampler */
	FShaderResourceParameter FluidTextureParameter;
	FShaderResourceParameter FluidTextureSamplerParameter;
};

/** Vertex Factory */
class FLUIDSURFACEENGINE_API FFluidSurfaceVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE( FFluidSurfaceVertexFactory );

public:

	/** Default constructor */
	FFluidSurfaceVertexFactory( ) {}

	/** Initialization */
	void Init( const FFluidSurfaceVertexBuffer* VertexBuffer );

	/** Modify Compilation Environment */
	static void ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment );

	/** Construct Shader Parameters */
	static FVertexFactoryShaderParameters* ConstructShaderParameters( EShaderFrequency ShaderFrequency );

	/* Should cache */
	static bool ShouldCache( EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType );

	/* Copy */
	void Copy( const FFluidSurfaceVertexFactory& Other );
};

/** Number of threads to use */
#define FLUID_SURFACE_THREAD_COUNT 32

/** Rarely changing Uniform buffer */
BEGIN_UNIFORM_BUFFER_STRUCT( FFluidSurfaceParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( int32, FluidGridType )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( int32, FluidXSize )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( int32, FluidYSize )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, FluidGridSpacing )
END_UNIFORM_BUFFER_STRUCT( FFluidSurfaceParameters )

typedef TUniformBufferRef<FFluidSurfaceParameters> FFluidSurfaceUniformBufferRef;

/** Frame changing Uniform Buffer */
BEGIN_UNIFORM_BUFFER_STRUCT( FFluidSurfaceFrameParameters, )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, UpdateRate )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, FluidNoiseStrengthMin )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, FluidNoiseStrengthMax )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, FluidSpeed )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, DeltaTime )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, Time )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( FVector4, FluidOriginDamping )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( float, FluidNoiseFrequency )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, LatestVerts )
	DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER( uint32, NumPLings )
END_UNIFORM_BUFFER_STRUCT( FFluidSurfaceFrameParameters )

typedef TUniformBufferRef<FFluidSurfaceFrameParameters> FFluidSurfaceFrameUniformBufferRef;

/** Compute Shader */
class FLUIDSURFACEENGINE_API FFluidSurfaceCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE( FFluidSurfaceCS, Global );

public:

	/** Default constructor */
	FFluidSurfaceCS( ) {}

	/** Initialization constructor */
	explicit FFluidSurfaceCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer );

	/** Cache only when using SM5 */
	static bool ShouldCache( EShaderPlatform Platform ) { return IsFeatureLevelSupported( Platform, ERHIFeatureLevel::SM5 ); }

	/** Modify compilation environment */
	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment );

	/** Serialization */
	virtual bool Serialize( FArchive& Ar ) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize( Ar );

		Ar << OutFluidSurface
			<< InFluidVerts0
			<< InFluidVerts1
			<< InFluidPLingBuffer;

		return bShaderHasOutdatedParams;
	}

	/** Set output buffers */
	void SetOutput( FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutFluidSurfaceUAV );

	/** Sets uniform buffers */
	void SetUniformBuffers( FRHICommandList& RHICmdList, FFluidSurfaceParameters& FluidParameters, FFluidSurfaceFrameParameters& FluidFrameParameters );

	/** Set input parameters */
	void SetParameters( 
		FRHICommandList& RHICmdList,
		FUnorderedAccessViewRHIRef FluidVerts0UAV, 
		FUnorderedAccessViewRHIRef FluidVerts1UAV, 
		FShaderResourceViewRHIRef FluidPLingSRV 
		);

	/** Unbinds bound buffers */
	void UnbindBuffers( FRHICommandList& RHICmdList );

private:

	/* Output buffer */
	FShaderResourceParameter OutFluidSurface;

	/* Flip/Flop Fluid Buffers */
	FShaderResourceParameter InFluidVerts0;
	FShaderResourceParameter InFluidVerts1;

	/* PLing Structured Buffer */
	FShaderResourceParameter InFluidPLingBuffer;
};

/** Data needed for the fluid surface vertex factory */
struct FFluidSurfaceBatchElementParams
{
	/* Default constructor */
	FFluidSurfaceBatchElementParams( ) {  }

	/* Scene Proxy */
	FTexture2DRHIRef FluidTextureResource;
};
