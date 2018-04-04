
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "GameFramework/Actor.h"

#include "FluidSurfaceActor.generated.h"

UCLASS( HideCategories = ( Input, Replication, Physics ) )
class AFluidSurfaceActor : public AActor
{
	GENERATED_UCLASS_BODY( )

	UPROPERTY( Category = FluidSurface, VisibleAnywhere, BlueprintReadOnly )
	class UFluidSurfaceComponent* FluidSurfaceComponent;
};

UENUM( )
namespace EFluidGridType
{
enum Type
{
	FGT_Square = 0,
	FGT_Hexagonal = 1,
	FGT_MAX = 2,
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