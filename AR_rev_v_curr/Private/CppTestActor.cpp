// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "CppTestActor.h"


// Sets default values
ACppTestActor::ACppTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACppTestActor::BeginPlay()
{
	Super::BeginPlay();
	ElapsedTime = 0.0;


	

	
}

// Called every frame
void ACppTestActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	//ApplyPitchRotation(AngVelocity);

	


	/*FVector NewLocation = GetActorLocation();

	float DeltaHeight = (FMath::Sin(ElapsedTime + DeltaTime) - FMath::Sin(ElapsedTime));

	NewLocation.Z += DeltaHeight;*/

	ElapsedTime += DeltaTime;

	//SetActorLocation(NewLocation);
}


void ACppTestActor::RotateAroundAxis(const FVector &Axis,float AngularVelocity, float dt)
{
	// create Quaternion with current Actor rotation
	//MyQuaternion = FQuat(GetActorRotation());

	// create an Axis (FVector) the Actor is going to be turning around
	//RotationAxis = Axis;
	//RotationAxis.Normalize();
	
	// rotate the Quaternion around the given Axis with a given Velocity, smoothing with DeltaTime
	//MyQuaternion *= FQuat(RotationAxis, FMath::DegreesToRadians(dt *  AngularVelocity));

	// convert back to Rotator and apply to the Actor
	//SetActorRotation(MyQuaternion.Rotator());

	//in short:
	//MyQuaternion = FQuat(GetActorRotation()) * FQuat(Axis.GetSafeNormal(), FMath::DegreesToRadians(dt *  AngularVelocity));
	//SetActorRotation(MyQuaternion.Rotator());

	// one line:
	SetActorRotation((FQuat(GetActorRotation()) * FQuat(Axis.GetSafeNormal(), FMath::DegreesToRadians(dt *  AngularVelocity))).Rotator());
}
