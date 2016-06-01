// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AR_rev_v_curr.h"
#include "Engine.h"
#include "GameFramework/DamageType.h"
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

	UFUNCTION(BlueprintCallable, Category = "Missile")
		virtual void MissileHit();
	UFUNCTION(NetMulticast, Reliable)
		void ServerMissileHit();


	UFUNCTION()
		void Explode();

	UFUNCTION(BlueprintNativeEvent, Category = "Missile")
		void HitTarget(class AActor* TargetedActor);

	UFUNCTION(BlueprintCallable, Category = "Missile")
		void OverlappingATarget(class AActor* OtherActor/*, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult*/);

	/** StaticMesh component that will be the visuals for the missile */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* MissileMesh;
	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return MissileMesh; }

	UPROPERTY(Category = ActorDetection, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* ActorDetectionSphere;

	UPROPERTY(Category = ParticleSystem, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* SmokeTrailTick;

	UPROPERTY(Category = ParticleSystem, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		UParticleSystemComponent* MissileTrail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile | Trail")
		float MissileTrailLifeSpan = 3.0f;

	UPROPERTY(Category = ParticleSystem, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* Explosion;

	UPROPERTY(Category = Sound, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* ExplosionSound;

	UPROPERTY(Category = Sound, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* MissileEngineSound;

	// Sets default values for this actor's properties
	AMissile(const FObjectInitializer& ObjectInitializer);

	/**	 Perform target location prediction*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Missile")
		FVector LinearTargetPrediction(
			const FVector &TargetLocation,
			const FVector &StartLocation,
			const FVector &TargetVelocity,
			const float ProjectileVelocity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float DamageMin = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float DamageMax = 110.0f;

	/** missile turnrate in deg/s*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float MaxTurnrate = 110.0f;

	/** missile velocity in cm/s*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float MaxVelocity = 2100.0f;

	/** missile velocity in cm/s*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
	float InitialVelocity = 1000.0f;



	/** time it takes for the missile to reach max velocity*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float AccelerationTime = 1.0f;

	/** distance to target where prediction is working at full strength */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float AdvancedMissileMinRange = 5000.0f;

	/** distance to target where prediction will be deactivated */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float AdvancedMissileMaxRange = 15000.0f;

	/** missile range in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float Range = 50000.0f;

	/** distance to target at which missile will explode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float TargetDetectionRadius = 50.0f;


	/** if set to false the missile will perform no homing */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool MissileLock = true;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool bBombingMode = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		FVector BombingTargetLocation;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
	class USceneComponent* CurrentTarget;

	/** is advanced homing active */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool AdvancedHoming = false;

	/** is spiral homing active */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool SpiralHoming = false;

	/** values between 1 to 360 degrees, leave at 0 do use random offset instead */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float CustomSpiralOffset = 0.0f;

	/** set to -1 for ccw and +1 for cw rotation, leave at 0 for random rotation*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float SpiralDirection = 0.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		bool bHit = false;


	/** Spiral strength factor */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float SpiralStrength = 0.2f;

	/** if set to true: given Velocity will be multiplied by a random factor (0.5 to 1.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool RandomizeSpiralVelocity = true;

	/** spiraling velocity in deg/s */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float SpiralVelocity = 180.0f;

	/** distance to target when spiraling deactivates to hit the target */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Missile")
		float SpiralDeactivationDistance = 1000.0f;


	///** A Replicated Boolean Flag */
	//UPROPERTY(Replicated)
	//	uint32 bFlag : 1;

	UPROPERTY(ReplicatedUsing = OnRep_MissileTransformOnAuthority, EditAnywhere, BlueprintReadWrite, Category = "Missile")
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

	UFUNCTION()
		void MissileMeshOverlap(class UPrimitiveComponent* ThisComp, class AActor* OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
		void MissileDestruction(AActor * actor);

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
	UFUNCTION()
		void OnRep_MissileTransformOnAuthority();

	bool bNotFirstTick = false;
	bool bDamageTarget = false;

	float MaxFlightTime;
	float LifeTime;

	float NetUpdateInterval;

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
	FVector ClientLocationError;
	float LocationCorrectionTimeLeft;
	FVector TargetVelocity;
	FVector PredictedTargetLocation;
	FVector HomingLocation;
	int32 FramesSinceLastVelocityCheck;
	/** perform homing to the target by rotating*/
	UFUNCTION()
		void Homing(float DeltaTime);

	UFUNCTION()
		void EnableAcceleration();
	bool bCanAccelerate = false;
	FDateTime currentTime;
	float Ping;
	float DistanceToTarget;
	float LastDistanceToTarget = TargetDetectionRadius;
	float AdvancedHomingStrength;
	UFUNCTION()
		float DistanceLineLine(const FVector& a1,
			const FVector& a2,
			const FVector& b1,
			const FVector& b2);
	UFUNCTION()
		bool ClosestPointsOnTwoLines(const FVector& LineStartA,
			const FVector& LineEndA,
			const FVector& LineStartB,
			const FVector& LineEndB,
			FVector& PointA,
			FVector& PointB);





};
