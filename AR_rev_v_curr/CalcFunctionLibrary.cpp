// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "CalcFunctionLibrary.h"
#include "KismetMathLibrary.generated.h"


//Test
FRotator UCalcFunctionLibrary::Fussel(FRotator Old)
{
	Old.Add(123, 123, 123);
	return Old;
}
//Test
FRotator UCalcFunctionLibrary::FusselAct(FRotator Old)
{
	return Old.Add(123, 123, 123);
}
//01
float UCalcFunctionLibrary::AngleBetween2Vectors(FVector Vector1, FVector Vector2)
{
	Vector1.Normalize();
	Vector2.Normalize();
	return FMath::RadiansToDegrees(acosf(FVector::DotProduct(Vector1, Vector2)));
}
//02
float UCalcFunctionLibrary::DistanceBetween2Locations(
	const FVector &Location1,
	const FVector &Location2)
	//returns a float with the distance between two locations
{
	return FVector::Dist(Location2, Location1);
}
//03
//modifies a,b,c to those of this function f(x)= ax^2 + bx + c
void UCalcFunctionLibrary::QuadReg(
	const float x1,
	const float x2,
	const float x3,
	const float y1,
	const float y2,
	const float y3,
	float &a,
	float &b,
	float &c)
{
	float tmp1 = (x1 - x2)*(x1 - x3); //small optimization
	float tmp2 = x2 - x3; //small optimization
	a = (x1*(y2 - y3) + x2*(y3 - y1) + x3*(y1 - y2)) / (tmp1*(x3 - x2));
	b = (x1*x1*(y2 - y3) + x2*x2*(y3 - y1) + x3*x3*(y1 - y2)) / (tmp1*tmp2);
	c = (x1*x1*(x2*y3 - x3*y2) + x1*(x3*x3*y2 - x2*x2*y3) + x2*x3*y1*tmp2) / (tmp1*tmp2);
}
//04
bool UCalcFunctionLibrary::IntersectLineWithParabel(
	float ALin,
	float BLin,
	float AQuad,
	float BQuad,
	float CQuad,
	float &TimeOfIntersection)
{
	float ValueForSqrt = FMath::Pow(((-ALin + BQuad) / (2.0f*AQuad)), 2.0f) - ((-BLin + CQuad) / AQuad);
	//prevent a negative value inside the sqrt
	if (ValueForSqrt < 0.f)
		return false;

	//x[1,2] = -p/2 +- sqrt((p/2)^2 - q)
	//		 |presqrt|
	float PreSqrt = -((-ALin + BQuad) / (2.0f*AQuad));

	if ((TimeOfIntersection = PreSqrt - FMath::Sqrt(ValueForSqrt)), TimeOfIntersection < 0.f)
		TimeOfIntersection = PreSqrt + FMath::Sqrt(ValueForSqrt);
	//a positive value for the intersectiontime is found	
	return true;
}
//05
float UCalcFunctionLibrary::ProjectileThickness(
	const FVector &ProjectileLocation,
	const FVector &CameraLocation,
	const float &FOV,
	const float &SizeFactor,
	const float &MaxGrowingDistance,
	const float &ProjectileThickness)
	//return the thickness (of projectiles) multiplied by a factor based on distance to camera and onscreensize
{
	//V1
	//float DeltaAngle = FMath::DegreesToRadians(FMath::Atan(FOV * SizeFactor));
	//float ScaleValue = 1.f + DistanceBetween2Locations(ProjectileLocation, CameraLocation) * DeltaAngle;
	//return ProjectileThickness * FMath::Clamp(ScaleValue, 1.f, MaxGrowingDistance * DeltaAngle);
	//V2
	//float DeltaAngle = 0.5f * FOV * SizeFactor;
	//float ScaleValue = 0.5f + DistanceBetween2Locations(ProjectileLocation, CameraLocation) * FMath::Tan(DeltaAngle);
	//return 2.f * ProjectileThickness * ScaleValue;
	//V3
	//return ProjectileThickness * (1.f + FVector::Dist(ProjectileLocation, CameraLocation) * FMath::Tan(FOV * SizeFactor));
	//V4
	return (FMath::Clamp<float>(FVector::Dist(ProjectileLocation, CameraLocation), 1.f, MaxGrowingDistance) * FMath::Tan(FOV * SizeFactor));
}

//06
FVector UCalcFunctionLibrary::ProjectileScale(
	const FVector &OldLocation,
	const FVector &NewLocation,
	const FVector &CameraLocation,
	const float ProjectileMeshLengthInCm,
	const float TracerLengthFactor,
	const float FOV,
	const float SizeFactor,
	const float MaxGrowingDistance,
	const float ProjectileThickness)
{
	//Tracer Thickness (in this case it is axis x an y)
	float Thickness = UCalcFunctionLibrary::ProjectileThickness(
		NewLocation,
		CameraLocation,
		FOV,
		SizeFactor,
		MaxGrowingDistance,
		ProjectileThickness);
	//distance between old and current projectile location
	float dist = FVector::Dist(OldLocation, NewLocation);

	//prevent the tracer from being shorter then thick
	if ((dist * TracerLengthFactor) > Thickness)
		return FVector::FVector(Thickness, Thickness, dist / ProjectileMeshLengthInCm * TracerLengthFactor);
	return FVector::FVector(Thickness, Thickness, Thickness / ProjectileMeshLengthInCm);
}
//07
FVector UCalcFunctionLibrary::LinearTargetPrediction(
	const FVector &TargetLocation,
	const FVector &StartLocation,
	const FVector &TargetVelocity,
	const float ProjectileVelocity)
	//returns a location at which has to be aimed in order to hit the target
{
	FVector ABmag = TargetLocation - StartLocation;
	ABmag.Normalize();
	FVector vi = TargetVelocity - (FVector::DotProduct(ABmag, TargetVelocity) * ABmag);
	//FVector vj = ABmag * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()),2.f));
	//return StartLocation + vi + vj;

	//in short:
	return StartLocation + vi + ABmag * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()), 2.f));
}
//08
FRotator UCalcFunctionLibrary::DetermineTurnDirection(
	const FVector &TargetLocation,
	const FVector &ActorLocation,
	const FVector &ActorUpVector,
	const FVector &ActorRightVector,
	const float DeltaSeconds,
	const float TurnrateDegreePerSecond)
{
	FVector Direction = TargetLocation - ActorLocation;
	float UpAngel = UCalcFunctionLibrary::AngleBetween2Vectors(ActorUpVector, Direction);
	float RightAngel = UCalcFunctionLibrary::AngleBetween2Vectors(ActorRightVector, Direction);

	if (UpAngel < 90.0f){
		UpAngel = 90.0f - UpAngel;
	}
	else{
		UpAngel = -(UpAngel - 90.0f);
	}

	if (RightAngel < 90.0f){
		RightAngel = 90.0f - RightAngel;
	}
	else{
		RightAngel = -(RightAngel - 90.0f);
	}

	float Mag = FMath::Sqrt(UpAngel*UpAngel + RightAngel*RightAngel);

	if (TurnrateDegreePerSecond*DeltaSeconds > Mag){
		return FRotator(DeltaSeconds * UpAngel,
			DeltaSeconds * RightAngel,
			0.f);
	}
	else {
		return FRotator(TurnrateDegreePerSecond / Mag * DeltaSeconds * UpAngel,
			TurnrateDegreePerSecond / Mag * DeltaSeconds * RightAngel,
			0.f);
	}
}
//09
float UCalcFunctionLibrary::FInterpFromToInTime(float Beginning, float Current, float Target, float DeltaTime, float InterpTime)
{
	// If no interp speed, jump to target value
	if (InterpTime == 0.f)
	{
		return Target;
	}

	// Distance left
	//10 - 11 = -1
	const float DeltaDist = Target - Current;
	// all time distance
	//10 - 0 = 10
	const float Dist = Target - Beginning;

	if (DeltaDist*Dist < 0.f)
		return Target;

	// If deltadistance is too small, just set the desired location
	if (FMath::Square(DeltaDist) < SMALL_NUMBER)
	{
		return Target;
	}

	// Delta Move, Clamp so we do not over shoot.
	//10 * 0.1 * 1/10 = 
	//10 * 0.01 = 0.1
	const float DeltaMove = Dist * DeltaTime * 1.f / InterpTime;

	//the new current value will ALWAYS be between beginning an target!
	return FMath::Clamp<float>(Current + DeltaMove, Beginning, Target);
}

//10
bool UCalcFunctionLibrary::TracerMotionBlur(
	APlayerController* Player,
	USceneComponent *BPProjectileType,
	const FVector2D &OldScreenLocation,
	FVector2D &NewScreenLocation,
	const FVector &OldProjectileLocation,
	const FVector &NewProjectileLocation,
	FVector &MovedOldProjectileLocation,
	bool FoundNewScreenLocation)
{
	Player->GetWorld()->GetFirstPlayerController();
	bool FoundWorldLocation = false;
	if (FoundNewScreenLocation) {

		//A

		//APlayerController::DeprojectScreenPositionToWorld(float ScreenX, float ScreenY, FVector& WorldLocation, FVector& WorldDirection)

		ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);

		if (LocalPlayer && LocalPlayer->ViewportClient && LocalPlayer->ViewportClient->Viewport)
		{
			// Create a view family for the game viewport
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
				LocalPlayer->ViewportClient->Viewport,
				LocalPlayer->GetWorld()->Scene,
				LocalPlayer->ViewportClient->EngineShowFlags)
				.SetRealtimeUpdate(true));

			// Calculate a view where the player is to update the streaming from the players start location
			FVector ViewLocation;
			FRotator ViewRotation;
			FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, LocalPlayer->ViewportClient->Viewport);
			FVector WorldLocation;
			FVector WorldDirection;
			if (SceneView)
			{
				//B

				const FVector2D ScreenPosition(OldScreenLocation.X, OldScreenLocation.Y);
				SceneView->DeprojectFVector2D(ScreenPosition, WorldLocation, WorldDirection);

				//C

				MovedOldProjectileLocation = FMath::ClosestPointOnLine(WorldLocation, WorldLocation + (WorldDirection * (100.f * FVector::Dist(OldProjectileLocation, WorldLocation))), OldProjectileLocation);

				FRotator TracerRotation = FRotationMatrix::MakeFromX(MovedOldProjectileLocation - OldProjectileLocation).Rotator();

				TracerRotation.Pitch += 90.f;

				BPProjectileType->SetWorldRotation(TracerRotation);
				FoundWorldLocation = true;
			}
		}
	}

	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
	if (LocalPlayer != NULL && LocalPlayer->ViewportClient != NULL && LocalPlayer->ViewportClient->Viewport != NULL)
	{
		// Create a view family for the game viewport
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			LocalPlayer->ViewportClient->Viewport,
			LocalPlayer->GetWorld()->Scene,
			LocalPlayer->ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(true));

		// Calculate a view where the player is to update the streaming from the players start location
		FVector ViewLocation;
		FRotator ViewRotation;
		FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, LocalPlayer->ViewportClient->Viewport);

		if (SceneView)
		{
			return SceneView->WorldToPixel(NewProjectileLocation, NewScreenLocation);
		}
		else {
			NewScreenLocation = FVector2D(0.f, 0.f);
		}
	}


	if (!FoundWorldLocation)
		MovedOldProjectileLocation = OldProjectileLocation;


	return FoundWorldLocation;
}

//11
float UCalcFunctionLibrary::controller_PID(
const float Value,
const float Target,
const float OldError,
float &NewError,
const float P,
const float I,
const float old_I_Term,
float &ITerm,
const float D,
const float DeltaTime) {

	NewError = Target - Value;

	float PTerm = P * NewError;

	ITerm = I * NewError  + old_I_Term;

	float DTerm = NewError / OldError * D;

	return PTerm + ITerm + DTerm;

}

