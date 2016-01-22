// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AR_rev_v_curr.h"
#include "Engine.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "Missile.generated.h"


UCLASS()
class AR_REV_V_CURR_API AMissile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMissile();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// called in editor for calcuation of missing values with dependencies
	void PostInitProperties();
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);




	/**	 return current lifetime of the missile in seconds*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Missile")
		float GetMissileLifetime();

	/**	 Perform target location prediction*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Missile")
		FVector LinearTargetPrediction(
			const FVector &TargetLocation,
			const FVector &StartLocation,
			const FVector &TargetVelocity,
			const float ProjectileVelocity);

	/** missile turnrate in deg/s*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float MaxTurnrate = 110.0f;

	/** missile velocity in cm/s*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float MaxVelocity = 4200.0f;

	/** time it takes for the missile to reach max velocity*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float AccelerationTime = 1.0f;

	/** distance to target where prediction is working at full strength */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float AdvancedMissileMinRange = 5000.0f;

	/** distance to target where prediction will be deactivated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float AdvancedMissileMaxRange = 15000.0f;

	/** missile range in m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float Range = 4000.0f;

	/** missile explosionradius in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float ExplosionRadius = 500.0f;	

	///** missile explosioneffect */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
	//	class UParticleSystemComponent* ExplosionEffect;
	
	/** the current target (scenecpmponent) the missile is homing towards */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
	class USceneComponent* CurrentTarget;

	/** is advanced homing active */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		bool AdvancedHoming = true;

	///** A Replicated Boolean Flag */
	//UPROPERTY(Replicated)
	//	uint32 bFlag : 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		FTransform MissileTransformOnAuthority;

	/** A Replicated Array Of Integers */
	UPROPERTY(Replicated)
		TArray<uint32> IntegerArray;


	/** A Replicated Boolean Flag */
	UPROPERTY(ReplicatedUsing = OnRep_Flag)
		uint32 bFlag : 1;

	void ServerSetFlag();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_RunsOnServer();

	UPROPERTY()
	APlayerState* State;

	UFUNCTION(NetMulticast, Reliable)
		void ServerDealing();
	virtual void Dealing();

	UFUNCTION(NetMulticast, Unreliable)
		void ServerRunsOnAllClients();
	virtual void RunsOnAllClients();

	////// example for function replication------------------------
	UPROPERTY(Replicated)
	bool bSomeBool = false;

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetSomeBool(bool bNewSomeBool);
	virtual void SetSomeBool(bool bNewSomeBool); // executed on client
	
	////// end: example for function replication------------------------

	////// example for function replication

	UFUNCTION(Client, Reliable)
		void Client_RunsOnOwningClientOnly();
	virtual void RunsOnOwningClientOnly();

	////// end: example for function replication

private:

	UFUNCTION()
		void OnRep_Flag();
	bool bNotFirstTick = false;

	float MaxLifeTime;
	float LifeTime;
	FVector ExplosionLocation;
	FVector RotationAxisForTurningToTarget;
	FVector NewDirection;
	FVector MovementVector;
	float AngleToTarget;
	float Acceleration;
	bool bReachedMaxVelocity = false;
	float Velocity;
	float Dot;
	float Turnrate;
	FVector DirectionToTarget;
	FVector CurrentTargetLocation;	
	FVector LastTargetLocation;
	FVector LastActorLocation;
	FVector TargetVelocity;
	FVector PredictedTargetLocation;
	int FramesSinceLastVelocityCheck;
	/** perform homing to the target by rotating*/
	void Homing(float DeltaTime);
	FDateTime currentTime;
	float Ping;
	float DistanceToTarget;
	float LastDistanceToTarget = ExplosionRadius;
	float AdvancedHomingStrength;	

	float DistanceLineLine(const FVector& a1,
		const FVector& a2,
		const FVector& b1,
		const FVector& b2);
	bool ClosestPointsOnTwoLines(const FVector& LineStartA,
		const FVector& LineEndA,
		const FVector& LineStartB,
		const FVector& LineEndB,
		FVector& PointA,
		FVector& PointB);
};
