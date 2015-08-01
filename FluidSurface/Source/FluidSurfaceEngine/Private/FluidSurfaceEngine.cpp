
#include "FluidSurfaceEnginePrivatePCH.h"

#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

IMPLEMENT_UNIFORM_BUFFER_STRUCT( FFluidSurfaceParameters, TEXT( "FluidSurfaceParams" ) )
IMPLEMENT_UNIFORM_BUFFER_STRUCT( FFluidSurfaceFrameParameters, TEXT( "FluidSurfaceFrameParams" ) )

/** Vertex Factory Shader Params */

void FFluidSurfaceVertexFactoryShaderParameters::Bind( const FShaderParameterMap& ParameterMap )
{
	FluidTextureParameter.Bind( ParameterMap, TEXT( "FluidTexture" ) );
	FluidTextureSamplerParameter.Bind( ParameterMap, TEXT( "FluidTextureSampler" ) );
}

void FFluidSurfaceVertexFactoryShaderParameters::SetMesh( FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags ) const
{
	/* Get batch element params */
	FFluidSurfaceBatchElementParams* BatchElementParams = (FFluidSurfaceBatchElementParams*) BatchElement.UserData;
	check( BatchElementParams );

	/* Get scene proxy */
	FTexture2DRHIRef FluidTextureResource = BatchElementParams->FluidTextureResource;

	if( FluidTextureParameter.IsBound( ) && FluidTextureResource )
	{
		if( Shader->GetTarget( ).Frequency == SF_Vertex )
		{
			SetTextureParameter(
				RHICmdList,
				Shader->GetVertexShader( ),
				FluidTextureParameter,
				FluidTextureSamplerParameter,
				TStaticSamplerState< SF_Point, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI( ),
				FluidTextureResource
				);
		}
		else if( Shader->GetTarget( ).Frequency == SF_Domain )
		{
			SetTextureParameter(
				RHICmdList,
				(FDomainShaderRHIParamRef) Shader->GetDomainShader( ),
				FluidTextureParameter,
				FluidTextureSamplerParameter,
				TStaticSamplerState< SF_AnisotropicLinear, AM_Clamp, AM_Clamp, AM_Clamp >::GetRHI( ),
				FluidTextureResource
				);
		}
	}
}

/** Vertex Factory */

void FFluidSurfaceVertexFactory::Init( const FFluidSurfaceVertexBuffer* VertexBuffer )
{
	if( IsInRenderingThread( ) )
	{
		DataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Position, VET_Float3 );
		NewData.TextureCoordinates.Add( FVertexStreamComponent( VertexBuffer, STRUCT_OFFSET( FFluidSurfaceVertex, TextureCoordinate ), sizeof( FFluidSurfaceVertex ), VET_Float2 ) );
		NewData.TangentBasisComponents[ 0 ] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Tangent, VET_Float3 );
		NewData.TangentBasisComponents[ 1 ] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Normal, VET_Float3 );

		SetData( NewData );
	}
	else
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitFluidSurfaceVertexFactory,
			FFluidSurfaceVertexFactory*, VertexFactory, this,
			const FFluidSurfaceVertexBuffer*, VertexBuffer, VertexBuffer,
			{
			// Initialize the vertex factory's stream components
			DataType NewData;
			NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Position, VET_Float3 );
			NewData.TextureCoordinates.Add( FVertexStreamComponent( VertexBuffer, STRUCT_OFFSET( FFluidSurfaceVertex, TextureCoordinate ), sizeof( FFluidSurfaceVertex ), VET_Float2 ) );
			NewData.TangentBasisComponents[ 0 ] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Tangent, VET_Float3 );
			NewData.TangentBasisComponents[ 1 ] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT( VertexBuffer, FFluidSurfaceVertex, Normal, VET_Float3 );

			VertexFactory->SetData( NewData );
		} );
	}
}

void FFluidSurfaceVertexFactory::ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
{
	//FVertexFactory::ModifyCompilationEnvironment( Platform, Material, OutEnvironment );
	OutEnvironment.SetDefine( TEXT( "USE_FLUIDSURFACE" ), TEXT( "1" ) );
}

bool FFluidSurfaceVertexFactory::ShouldCache( EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType )
{
	// @todo
	return true;
}

void FFluidSurfaceVertexFactory::Copy( const FFluidSurfaceVertexFactory& Other )
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FFluidSurfaceVertexFactoryCopyData,
		FFluidSurfaceVertexFactory*, VertexFactory, this,
		const DataType*, DataCopy, &Other.Data,
		{
		VertexFactory->Data = *DataCopy;
	} );

	BeginUpdateResourceRHI( this );
}

FVertexFactoryShaderParameters* FFluidSurfaceVertexFactory::ConstructShaderParameters( EShaderFrequency ShaderFrequency )
{
	if( ShaderFrequency == SF_Vertex || ShaderFrequency == SF_Domain )
		return new FFluidSurfaceVertexFactoryShaderParameters( );

	return NULL;
}

IMPLEMENT_VERTEX_FACTORY_TYPE( FFluidSurfaceVertexFactory, "FluidSurfaceVertexFactory", true, true, true, true, false );

/** Compute Shader */

FFluidSurfaceCS::FFluidSurfaceCS( const ShaderMetaType::CompiledShaderInitializerType& Initializer )
: FGlobalShader( Initializer )
{
	OutFluidSurface.Bind( Initializer.ParameterMap, TEXT( "OutFluidSurface" ) );
	InFluidVerts0.Bind( Initializer.ParameterMap, TEXT( "FluidVerts0" ) );
	InFluidVerts1.Bind( Initializer.ParameterMap, TEXT( "FluidVerts1" ) );
	InFluidPLingBuffer.Bind( Initializer.ParameterMap, TEXT( "FluidPLingBuffer" ) );
}

/* Sets flags to use during compilation of this shader */
void FFluidSurfaceCS::ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
{
	FGlobalShader::ModifyCompilationEnvironment( Platform, OutEnvironment );
	OutEnvironment.CompilerFlags.Add( CFLAG_StandardOptimization );
}

/* Sets the compute shader output parameter */
void FFluidSurfaceCS::SetOutput( FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutFluidSurfaceUAV )
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader( );
	if( OutFluidSurface.IsBound( ) )
		RHICmdList.SetUAVParameter( ComputeShaderRHI, OutFluidSurface.GetBaseIndex( ), OutFluidSurfaceUAV );
}

/* Sets compute shader uniform buffers */
void FFluidSurfaceCS::SetUniformBuffers( FRHICommandList& RHICmdList, FFluidSurfaceParameters& FluidParameters, FFluidSurfaceFrameParameters& FluidFrameParameters )
{
	FFluidSurfaceUniformBufferRef FluidUniformBuffer;
	FFluidSurfaceFrameUniformBufferRef FluidFrameUniformBuffer;

	FluidUniformBuffer = FFluidSurfaceUniformBufferRef::CreateUniformBufferImmediate( FluidParameters, UniformBuffer_SingleDraw );
	FluidFrameUniformBuffer = FFluidSurfaceFrameUniformBufferRef::CreateUniformBufferImmediate( FluidFrameParameters, UniformBuffer_SingleDraw );

	/* Set uniform buffers */
	SetUniformBufferParameter( RHICmdList, GetComputeShader( ), GetUniformBufferParameter<FFluidSurfaceParameters>( ), FluidUniformBuffer );
	SetUniformBufferParameter( RHICmdList, GetComputeShader( ), GetUniformBufferParameter<FFluidSurfaceFrameParameters>( ), FluidFrameUniformBuffer );
}

/* Sets compute shader parameters */
void FFluidSurfaceCS::SetParameters( 
	FRHICommandList& RHICmdList,
	FUnorderedAccessViewRHIRef FluidVerts0UAV,
	FUnorderedAccessViewRHIRef FluidVerts1UAV,
	FShaderResourceViewRHIRef FluidPLingSRV 
	)
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader( );

	/* Set fluid verts */
	if( InFluidVerts0.IsBound( ) )
		RHICmdList.SetUAVParameter( ComputeShaderRHI, InFluidVerts0.GetBaseIndex( ), FluidVerts0UAV );

	/* Set fluid verts */
	if( InFluidVerts1.IsBound( ) )
		RHICmdList.SetUAVParameter( ComputeShaderRHI, InFluidVerts1.GetBaseIndex( ), FluidVerts1UAV );

	/* Set pling buffer */
	if( InFluidPLingBuffer.IsBound( ) )
		RHICmdList.SetShaderResourceViewParameter( ComputeShaderRHI, InFluidPLingBuffer.GetBaseIndex( ), FluidPLingSRV );

}

/* Unbinds buffers that will be used elsewhere */
void FFluidSurfaceCS::UnbindBuffers( FRHICommandList& RHICmdList )
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader( );
	if( OutFluidSurface.IsBound( ) )
		RHICmdList.SetUAVParameter( ComputeShaderRHI, OutFluidSurface.GetBaseIndex( ), FUnorderedAccessViewRHIRef( ) );

	if( InFluidPLingBuffer.IsBound( ) )
		RHICmdList.SetShaderResourceViewParameter( ComputeShaderRHI, InFluidPLingBuffer.GetBaseIndex( ), FShaderResourceViewRHIRef( ) );
}

IMPLEMENT_SHADER_TYPE( , FFluidSurfaceCS, TEXT( "FluidSurfaceShader" ), TEXT( "ComputeFluidSurface" ), SF_Compute );

/** Fluid Surface triangles stat */
DEFINE_STAT( STAT_FluidSurfaceTriangles );

IMPLEMENT_MODULE( FDefaultModuleImpl, FluidSurfaceEngine )
