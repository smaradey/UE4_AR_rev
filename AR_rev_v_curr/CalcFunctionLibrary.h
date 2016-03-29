// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CalcFunctionLibrary.generated.h"

/**
 *
 */
UCLASS()
class AR_REV_V_CURR_API UCalcFunctionLibrary : public UObject //UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//Test
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Fusseln_C++")
		static FRotator Fussel(FRotator Old);
	//Test
	UFUNCTION(BlueprintCallable, Category = "Fusseln_C++")
		static FRotator FusselAct(FRotator Old);
	//01
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static float AngleBetween2Vectors(FVector Vector1, FVector Vector2);
	//02
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static float DistanceBetween2Locations(
		const FVector &Location1,
		const FVector &Location2);
	//03		
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static void QuadReg(
		const float x1,
		const float x2,
		const float x3,
		const float y1,
		const float y2,
		const float y3,
		float &a,
		float &b,
		float &c);
	//04
	//
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static bool IntersectLineWithParabel(
		float ALin,
		float BLin,
		float AQuad,
		float BQuad,
		float CQuad,
		float &TimeOfIntersection);


	//05
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile_C++")
		static float ProjectileThickness(
		const FVector &ProjectileLocation,
		const FVector &CameraLocation,
		const float &FOV,
		const float &SizeFactor = 0.001f,
		const float &MaxGrowingDistance = 15000.0f,
		const float &ProjectileThickness = 3.0f);
	//06
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile_C++")
		static FVector ProjectileScale(
		const FVector &OldLocation,
		const FVector &NewLocation,
		const FVector &CameraLocation,
		const float ProjectileMeshLengthInCm = 100.0f,
		const float TracerLengthFactor = 0.5f,
		const float FOV = 90.0f,
		const float SizeFactor = 0.001f,
		const float MaxGrowingDistance = 15000.0f,
		const float ProjectileThickness = 3.0f);
	//07
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Prediction_C++")
		static FVector LinearTargetPrediction(
		const FVector &TargetLocation,
		const FVector &StartLocation,
		const FVector &TargetVelocity,
		const float ProjectileVelocity = 0.f);
	//08
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement_C++")
		static FRotator DetermineTurnDirection(
		const FVector &TargetLocation,
		const FVector &ActorLocation,
		const FVector &ActorUpVector,
		const FVector &ActorRightVector,
		const float DeltaSeconds,
		const float TurnrateDegreePerSecond = 90.0f);
	//09
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interpolation_C++")
		static float FInterpFromToInTime(float Beginning, float Current, float Target, float DeltaTime, float InterpTime);
	//10
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile_C++")
		static bool TracerMotionBlur(
		APlayerController* Player,
		USceneComponent *BPProjectileType,

		const FVector2D &OldScreenLocation,
		FVector2D &NewScreenLocation,

		const FVector &OldProjectileLocation,
		const FVector &NewProjectileLocation,
		FVector &MovedOldProjectileLocation,

		bool FoundNewScreenLocation);

	//11
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static float controller_PID(
		const float Value,
		const float Target,
		const float OldError,
		float &NewError,
		const float P,
		const float I,
		const float old_I_Term,
		float &ITerm,
		const float D,
		const float DeltaTime);

	//12
	/* convert a given FOV to UE4 horizontal FOV */
	UFUNCTION(BlueprintCallable, Category = "Math_C++ | Camera")
		static void SetCameraFOV(const float newFOV, const bool bVertical, const bool bDiagonal, float &CameraFOV, const bool bHorizontal = true);
	
	UFUNCTION(BlueprintCallable, Category = "Actor | Replication")
		static void GetReplicatedMovement(AActor* Actor, FVector &LinearVelocity, FVector &AngularVelocity );



};
