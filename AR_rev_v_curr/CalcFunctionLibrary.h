// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AR_rev_v_curr.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CalcFunctionLibrary.generated.h"


/* helper struct to calculate velocities of objects, needs to be updated every tick */
USTRUCT(BlueprintType)
struct FVelocity
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(BlueprintReadWrite)
		FVector CurrentLocation;

	UPROPERTY(BlueprintReadWrite)
		FVector PreviousLocation;

	// initializes the values to a location, setting the velocity effectively to zero, prevents large values from occuring on the first read-out
	void Init(const FVector& Location)
	{
		PreviousLocation = Location;
		CurrentLocation = Location;
	}

	/**
	* updates the currentlocation to the new location and stores the current location in the previous location
	*/
	void SetCurrentLocation(const FVector& NewCurrentLocation)
	{
		PreviousLocation = CurrentLocation;
		CurrentLocation = NewCurrentLocation;
	}

	/** calculates the velocity [cm/s] from the current and the previous location and the delta-time
	* @param DeltaTime	delta-time between the last two Location updates
	*/
	FVector GetVelocityVector(const float DeltaTime)
	{
		// prevent division with zero
		if (DeltaTime == 0.0f) return FVector::ZeroVector;
		return (CurrentLocation - PreviousLocation) / DeltaTime;
	}
};

#define TwoDivPI (2.0f / PI)
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
			const FVector& Location1,
			const FVector& Location2);
	//03		
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static void QuadReg(
			const float x1,
			const float x2,
			const float x3,
			const float y1,
			const float y2,
			const float y3,
			float& a,
			float& b,
			float& c);
	//04
	//
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static bool IntersectLineWithParabel(
			float ALin,
			float BLin,
			float AQuad,
			float BQuad,
			float CQuad,
			float& TimeOfIntersection);


	//05
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile_C++")
		static float ProjectileThickness(
			const FVector& ProjectileLocation,
			const FVector& CameraLocation,
			const float MaxGrowingDistance = 15000.0f,
			const float ProjectileThickness = 3.0f);
	//06
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Projectile_C++")
		static FVector ProjectileScale(
			const FVector& OldLocation,
			const FVector& NewLocation,
			const FVector& CameraLocation,
			const float ProjectileMeshLengthInCm = 100.0f,
			const float TracerLengthFactor = 0.5f,
			const float MaxGrowingDistance = 15000.0f,
			const float ProjectileThickness = 3.0f);
	//07
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Prediction_C++")
		static FVector LinearTargetPrediction(
			const FVector& TargetLocation,
			const FVector& StartLocation,
			const FVector& TargetVelocity,
			const float ProjectileVelocity = 0.f);
	//08
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Movement_C++")
		static FRotator DetermineTurnDirection(
			const FVector& TargetLocation,
			const FVector& ActorLocation,
			const FVector& ActorUpVector,
			const FVector& ActorRightVector,
			const float DeltaSeconds,
			const float TurnrateDegreePerSecond = 90.0f);

	//10
	UFUNCTION(BlueprintCallable, Category = "Projectile_C++")
		static bool TracerMotionBlur(
			APlayerController* InPlayer,
			const FVector2D& InOldScreenPos,
			const bool& InHasScreenPos,
			FVector& OutWorldPosition,
			FVector& OutWorldDirection,
			FVector& OutLocationForOldScreenPos,
			const FVector& InOldProjectileLocation,
			const FVector& InNewProjectileLocation,
			FRotator& OutTracerRotation,
			FVector2D& OutScreenPosition);


	//11
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++")
		static float controller_PID(
			const float Value,
			const float Target,
			const float OldError,
			float& NewError,
			const float P,
			const float I,
			const float old_I_Term,
			float& ITerm,
			const float D,
			const float DeltaTime);

	//12
	/* convert a given FOV to UE4 horizontal FOV */
	UFUNCTION(BlueprintCallable, Category = "Math_C++ | Camera")
		static void SetCameraFOV(const float newFOV, const bool bVertical, const bool bDiagonal, float& CameraFOV, const bool bHorizontal = true);

	UFUNCTION(BlueprintCallable, Category = "Actor | Replication")
		static void GetReplicatedMovement(AActor* Actor, FVector& LinearVelocity, FVector& AngularVelocity);

	//12
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++ | Gravity")
		static FVector GetWorldGravity(UObject* other)
	{
		if (other && other->GetWorld())
		{
			return FVector(0, 0, other->GetWorld()->GetGravityZ());
		}
		return FVector::ZeroVector;
	}

	// 13 interpolate from A to B using Alpha [0.0..1.0] and the sinus function
	// Alpha will be clamped to 0.0..1.0
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math_C++ | Ease")
		static float FEaseInOutSin(const float A, const float B, const float Alpha);

	UFUNCTION(BlueprintCallable, Category = "Target Prediction")
		static void LinearTargetPredction(const FVector& TargetLocation, const FVector& StartLocation, FVelocity MainTargetVelocity, const float DeltaTime, const FVector& AdditionalProjectileVelocity, const float ProjectileVelocity, FVector& AimLocation)
	{
		const FVector TargetVelocity = MainTargetVelocity.GetVelocityVector(DeltaTime) - AdditionalProjectileVelocity;

		const FVector DirToTarget = (TargetLocation - StartLocation).GetSafeNormal();

		const FVector uj = TargetVelocity.ProjectOnToNormal(DirToTarget);
		//const float Dot = FVector::DotProduct(DirToTarget, TargetVelocity);
		//const FVector uj = Dot * DirToTarget;

		const FVector ui = TargetVelocity - uj;

		const FVector& vi = ui;	
		
		const float viMag = vi.Size();
		if(viMag > ProjectileVelocity)
		{
			// it is not possible for the projectile to hit the target (it is too slow).
			AimLocation = TargetLocation;
		}
		else {
			const float viMagSquare = viMag * viMag;
			const float vMagSquare = ProjectileVelocity * ProjectileVelocity;
			const float vj = FMath::Sqrt(vMagSquare - viMagSquare);
			AimLocation = StartLocation + vi + DirToTarget * vj;
		}
	}
};
