
#include "FluidSurfacePrivatePCH.h"

AFluidSurfaceModifier::AFluidSurfaceModifier(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FluidActor( NULL )
	, Active( true )
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent0"));
	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>( TEXT( "Sprite" ) );

	if( !IsRunningCommandlet( ) )
	{
		if( ( SpriteComponent != nullptr ) )
		{
			// Structure to hold one-time initialization
			struct FConstructorStatics
			{
				ConstructorHelpers::FObjectFinderOptional<UTexture2D> TextRenderTexture;
				FConstructorStatics( )
					: TextRenderTexture( TEXT( "/Engine/EditorResources/S_TextRenderActorIcon" ) )
				{
				}
			};
			static FConstructorStatics ConstructorStatics;

			SpriteComponent->Sprite = ConstructorStatics.TextRenderTexture.Get( );
			SpriteComponent->AttachParent = SceneComponent;
			SpriteComponent->bIsScreenSizeScaled = true;
			SpriteComponent->bAbsoluteScale = true;
			SpriteComponent->bReceivesDecals = false;
		}
	}
#endif
}

#if WITH_EDITOR
/** Pre Edit Change */
void AFluidSurfaceModifier::PreEditChange( UProperty* PropertyAboutToChange )
{
	if( PropertyAboutToChange )
	{
		const FName PropertyName = ( PropertyAboutToChange != NULL ) ? PropertyAboutToChange->GetFName( ) : NAME_None;

		if( PropertyName == GET_MEMBER_NAME_CHECKED( AFluidSurfaceModifier, FluidActor ) && FluidActor )
			FluidActor->FluidSurfaceComponent->Modifiers.Remove( this );
	}

	Super::PreEditChange( PropertyAboutToChange );
}

/** Post Edit Change Property */
void AFluidSurfaceModifier::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	if( PropertyChangedEvent.Property )
	{
		const FName PropertyName = ( PropertyChangedEvent.Property != NULL ) ? PropertyChangedEvent.Property->GetFName( ) : NAME_None;

		if( PropertyName == GET_MEMBER_NAME_CHECKED( AFluidSurfaceModifier, FluidActor ) && FluidActor )
			FluidActor->FluidSurfaceComponent->Modifiers.Add( this );
	}

	Super::PostEditChangeProperty( PropertyChangedEvent );
}
#endif

/** Destroy */
void AFluidSurfaceModifier::BeginDestroy( )
{
	Super::BeginDestroy( );

	if( FluidActor )
		FluidActor->FluidSurfaceComponent->Modifiers.Remove( this );
}