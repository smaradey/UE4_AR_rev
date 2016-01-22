// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "CppTestActor.generated.h"

UCLASS()
class AR_REV_V_CURR_API ACppTestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACppTestActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float dt ) override;

	// angular velocity in degree per second
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "C++ Testing")
	float AngVelocity = 45.0;

	UFUNCTION(BlueprintCallable, Category = "C++ Testing")
		void RotateAroundAxis(const FVector &Axis, float AngularVelocity, float dt);
		
	
private:
	
	FVector RotationAxis;
	FQuat MyQuaternion;

	float ElapsedTime;
};
