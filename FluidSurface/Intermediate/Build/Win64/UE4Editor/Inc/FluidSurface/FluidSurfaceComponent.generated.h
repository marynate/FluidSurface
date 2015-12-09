// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	C++ class header boilerplate exported from UnrealHeaderTool.
	This is automatically generated by the tools.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "ObjectBase.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct FVector;
class AActor;
class UPrimitiveComponent;
struct FHitResult;
#ifdef FLUIDSURFACE_FluidSurfaceComponent_generated_h
#error "FluidSurfaceComponent.generated.h already included, missing '#pragma once' in FluidSurfaceComponent.h"
#endif
#define FLUIDSURFACE_FluidSurfaceComponent_generated_h

#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_24_GENERATED_BODY \
	friend FLUIDSURFACE_API class UScriptStruct* Z_Construct_UScriptStruct_FRangedValues(); \
	FLUIDSURFACE_API static class UScriptStruct* StaticStruct();


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execPling) \
	{ \
		P_GET_STRUCT_REF(FVector,Z_Param_Out_Position); \
		P_GET_PROPERTY(UFloatProperty,Z_Param_Strength); \
		P_GET_PROPERTY(UFloatProperty,Z_Param_Radius); \
		P_FINISH; \
		this->Pling(Z_Param_Out_Position,Z_Param_Strength,Z_Param_Radius); \
	} \
 \
	DECLARE_FUNCTION(execComponentTouched) \
	{ \
		P_GET_OBJECT(AActor,Z_Param_Other); \
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OtherComp); \
		P_GET_PROPERTY(UIntProperty,Z_Param_OtherBodyIndex); \
		P_GET_UBOOL(Z_Param_bFromSweep); \
		P_GET_STRUCT_REF(FHitResult,Z_Param_Out_SweepResult); \
		P_FINISH; \
		this->ComponentTouched(Z_Param_Other,Z_Param_OtherComp,Z_Param_OtherBodyIndex,Z_Param_bFromSweep,Z_Param_Out_SweepResult); \
	}


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execPling) \
	{ \
		P_GET_STRUCT_REF(FVector,Z_Param_Out_Position); \
		P_GET_PROPERTY(UFloatProperty,Z_Param_Strength); \
		P_GET_PROPERTY(UFloatProperty,Z_Param_Radius); \
		P_FINISH; \
		this->Pling(Z_Param_Out_Position,Z_Param_Strength,Z_Param_Radius); \
	} \
 \
	DECLARE_FUNCTION(execComponentTouched) \
	{ \
		P_GET_OBJECT(AActor,Z_Param_Other); \
		P_GET_OBJECT(UPrimitiveComponent,Z_Param_OtherComp); \
		P_GET_PROPERTY(UIntProperty,Z_Param_OtherBodyIndex); \
		P_GET_UBOOL(Z_Param_bFromSweep); \
		P_GET_STRUCT_REF(FHitResult,Z_Param_Out_SweepResult); \
		P_FINISH; \
		this->ComponentTouched(Z_Param_Other,Z_Param_OtherComp,Z_Param_OtherBodyIndex,Z_Param_bFromSweep,Z_Param_Out_SweepResult); \
	}


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_INCLASS_NO_PURE_DECLS \
	private: \
	static void StaticRegisterNativesUFluidSurfaceComponent(); \
	friend FLUIDSURFACE_API class UClass* Z_Construct_UClass_UFluidSurfaceComponent(); \
	public: \
	DECLARE_CLASS(UFluidSurfaceComponent, UMeshComponent, COMPILED_IN_FLAGS(0), 0, FluidSurface, NO_API) \
	DECLARE_SERIALIZER(UFluidSurfaceComponent) \
	/** Indicates whether the class is compiled into the engine */    enum {IsIntrinsic=COMPILED_IN_INTRINSIC}; \
	virtual UObject* _getUObject() const override { return const_cast<UFluidSurfaceComponent*>(this); }


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_INCLASS \
	private: \
	static void StaticRegisterNativesUFluidSurfaceComponent(); \
	friend FLUIDSURFACE_API class UClass* Z_Construct_UClass_UFluidSurfaceComponent(); \
	public: \
	DECLARE_CLASS(UFluidSurfaceComponent, UMeshComponent, COMPILED_IN_FLAGS(0), 0, FluidSurface, NO_API) \
	DECLARE_SERIALIZER(UFluidSurfaceComponent) \
	/** Indicates whether the class is compiled into the engine */    enum {IsIntrinsic=COMPILED_IN_INTRINSIC}; \
	virtual UObject* _getUObject() const override { return const_cast<UFluidSurfaceComponent*>(this); }


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UFluidSurfaceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFluidSurfaceComponent) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UFluidSurfaceComponent); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFluidSurfaceComponent); \
private: \
	/** Private copy-constructor, should never be used */ \
	NO_API UFluidSurfaceComponent(const UFluidSurfaceComponent& InCopy); \
public:


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UFluidSurfaceComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private copy-constructor, should never be used */ \
	NO_API UFluidSurfaceComponent(const UFluidSurfaceComponent& InCopy); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UFluidSurfaceComponent); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFluidSurfaceComponent); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFluidSurfaceComponent)


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_41_PROLOG
#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_RPC_WRAPPERS \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_INCLASS \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_RPC_WRAPPERS_NO_PURE_DECLS \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_INCLASS_NO_PURE_DECLS \
	Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h_44_ENHANCED_CONSTRUCTORS \
static_assert(false, "Unknown access specifier for GENERATED_BODY() macro in class FluidSurfaceComponent."); \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID Engine_Plugins_Runtime_FluidSurface_Source_FluidSurface_Public_FluidSurfaceComponent_h


#define FOREACH_ENUM_EFLUIDGRIDTYPE(op) \
	op(EFluidGridType::FGT_Square) \
	op(EFluidGridType::FGT_Hexagonal) 
PRAGMA_ENABLE_DEPRECATION_WARNINGS
