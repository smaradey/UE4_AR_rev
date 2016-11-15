// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AR_rev_v_curr.h"
#include "Engine.h"
#include "GameFramework/DamageType.h"
#include "UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "CalcFunctionLibrary.h"
#include "Damage.h"
#include "Missile_Structs.h"
#include "Missile_Interface.h"
#define LOG_MSG 1
#include "CustomMacros.h"
#include "Target_Interface.h"
#include "Missile.generated.h"


UCLASS()

class AR_REV_V_CURR_API AMissile : public AActor, public IMissile_Interface
{
	GENERATED_BODY()

public:

	// Missile_Interface Implementations
	void Explode_Implementation(UObject* object) override;
	void DeactivateForDuration_Implementation(const float Duration) override;
	FMissileStatus GetCurrentMissileStatus_Implementation() override;

	// Constructor that sets default values for this actor's properties
	AMissile(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// called in editor for calculation of missing values with dependencies
	void PostInitProperties();
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);


	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		FMissileProperties mProperties;

private:

	EHomingType mHomingStatus;

	float mRemainingBoostDistance;
	// Decrease the distance the missile can still travel using boost
	void DecreaseRemainingDistance(const float DeltaTime, const float BoostIntensity = 1.0f);

	void MaxBoostRangeReached()
	{
		MissileHit();
	}

	void CheckTargetTargetable()
	{
		if(CurrentTarget)
		{
			AActor* TargetActor = CurrentTarget->GetOwner();
			if(TargetActor && TargetActor->GetClass()->ImplementsInterface(UTarget_Interface::StaticClass()))
			{
				if(!ITarget_Interface::Execute_GetIsTargetable(TargetActor, this))
				{
					CurrentTarget = nullptr;
				}
			}
		}
	}

public:
	/* TODO */
	UFUNCTION(BlueprintCallable, Category = "Missile")
		virtual void MissileHit();

	/* TODO */
	UFUNCTION(NetMulticast, Reliable)
		void ServerMissileHit();

	/* TODO */
	UFUNCTION()
		void ExplodeMissile();

	/* TODO */
	UFUNCTION(BlueprintNativeEvent, Category = "Missile")
		void HitTarget(class AActor* TargetedActor);

	/* TODO */
	UFUNCTION(BlueprintCallable, Category = "Missile")
		void OverlappingATarget(class AActor* OtherActor/*, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult*/);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mesh)
		USkeletalMeshComponent* Mesh;
	/** Returns PlaneMesh subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/* TODO */
	UPROPERTY(Category = ActorDetection, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		class USphereComponent* ActorDetectionSphere;

	/* particle system that gets attached to the missile mesh using the Socket "booster" if it exists */
	UPROPERTY(Category = ParticleSystem, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		UParticleSystemComponent* mMissileTrail;

	/* How long the particle system is still visible after it has been deactivated */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile | Trail")
		float mMissileTrailLifeSpan = 10.0f;

	/* particlesystem that is activated when the missile explodes */
	UPROPERTY(Category = ParticleSystem, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		class UParticleSystem* mExplosion;

	/* explosion sound effect */
	UPROPERTY(Category = Sound, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		class UAudioComponent* mExplosionSound;

	/* Sound Cue that loops while the missile is boosting */
	UPROPERTY(Category = Sound, BlueprintReadWrite, EditAnywhere, meta = (AllowPrivateAccess = "true"))
		class UAudioComponent* mBoosterSound;

	/** missile velocity in cm/s*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float InitialVelocity = 1000.0f;


	/** if set to false the missile will perform no homing */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool MissileLock = true;

	/* TODO */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		bool bBombingMode = false;

	/* TODO */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		FVector BombingTargetLocation;

	/* TODO */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		class USceneComponent* CurrentTarget;


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

	/* TODO */
	void ServerSetFlag();

	/* TODO */
	UFUNCTION(Server, Reliable, WithValidation)
		void Server_RunsOnServer();

	/* TODO */
	UPROPERTY()
		APlayerState* State;

	/* TODO */
	UFUNCTION(NetMulticast, Reliable)
		void ServerDealing();
	/* TODO */
	virtual void Dealing();

	/* TODO */
	UFUNCTION(NetMulticast, Unreliable)
		void ServerRunsOnAllClients();

	/* TODO */
	virtual void RunsOnAllClients();

	/* TODO */
	UFUNCTION()
		void MissileMeshOverlap(class UPrimitiveComponent* ThisComp, class AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/* TODO */
	UFUNCTION()
		void MissileDestruction(AActor* actor);

	////// example for function replication------------------------
	UPROPERTY(Replicated)
		bool bSomeBool = false;

	/* TODO */
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerSetSomeBool(bool bNewSomeBool);

	/* TODO */
	virtual void SetSomeBool(bool bNewSomeBool); // executed on client

	////// end: example for function replication------------------------

	////// example for function replication
	/* TODO */
	UFUNCTION(Client, Reliable)
		void Client_RunsOnOwningClientOnly();
	/* TODO */
	virtual void RunsOnOwningClientOnly();

	////// end: example for function replication

private:

	/* TODO */
	UFUNCTION()
		void OnRep_Flag();

	/* TODO */
	UFUNCTION()
		void OnRep_MissileTransformOnAuthority();

	/* TODO */
	bool bNotFirstTick = false;
	/* TODO */
	bool bDamageTarget = true;
	/* TODO */
	float MaxFlightTime;
	/* TODO */
	float LifeTime;
	/* TODO */
	float NetUpdateInterval;
	/* TODO */
	FVector RotationAxisForTurningToTarget;
	/* TODO */
	FVector NewDirection;
	/* TODO */
	FVector MovementVector;
	/* TODO */
	float AngleToTarget;
	/* TODO */
	float Acceleration;
	/* TODO */
	bool bReachedMaxVelocity = false;
	/* TODO */
	float Velocity;
	/* TODO */
	float Dot;
	/* TODO */
	float Turnrate;
	/* TODO */
	FVector DirectionToTarget;
	/* TODO */
	FVector CurrentTargetLocation;
	/* TODO */
	FVector LastTargetLocation;
	/* TODO */
	FVector LastActorLocation;
	/* TODO */
	FVector ClientLocationError;
	/* TODO */
	float LocationCorrectionTimeLeft;
	/* TODO */
	FVector TargetVelocity;
	/* TODO */
	FVector PredictedTargetLocation;
	/* TODO */
	FVector HomingLocation;
	/* TODO */
	int32 FramesSinceLastVelocityCheck;
	/** perform homing to the target by rotating*/
	UFUNCTION()
		void Homing(float DeltaTime);
	/* TODO */
	UFUNCTION()
		void EnableAcceleration();
	/* TODO */
	bool bCanAccelerate = false;
	/* TODO */
	FDateTime currentTime;
	/* TODO */
	float Ping;
	/* TODO */
	float DistanceToTarget;
	/* TODO */
	float LastDistanceToTarget = 500.0f;
	/* TODO */
	float AdvancedHomingStrength;
	/* TODO */
	UFUNCTION()
		float DistanceLineLine(const FVector& a1,
			const FVector& a2,
			const FVector& b1,
			const FVector& b2);
	/* TODO */
	UFUNCTION()
		bool ClosestPointsOnTwoLines(const FVector& LineStartA,
			const FVector& LineEndA,
			const FVector& LineStartB,
			const FVector& LineEndB,
			FVector& PointA,
			FVector& PointB);

	// sets the Vector-Parameter "SmokeDrift" of the Missile-Trail to simulate wind/turbulence that effects the smoke
	UFUNCTION()
		void SmokeDrift(const float DeltaTime);


	// target angle on the roll-axis of the missile in which direction the smoke will drift
	float SmokeRollAngleTarget = 0.0f;
	// how fast the roll angle will be interpolated to the target roll angle
	float SmokeRollInterpSpeed = 1.0f;

	float SmokeInterpAlpha = 1.0f;

	FVector CurrentInitialVelocityDirection;
	FVector TargetInitialVelocityDirection;
};
