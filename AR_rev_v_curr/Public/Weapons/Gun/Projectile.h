// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Damage.h"
#include "Projectile_Enums.h"
#include "Projectile_Structs.h"
#define LOG_MSG 0
#include "CustomMacros.h"
#include "CalcFunctionLibrary.h"
#include "Projectile.generated.h"

// custom collision profile created in Editor, Tracechannel found in DefaultEngine.ini
#define ECC_Projectile ECC_GameTraceChannel1


UCLASS()

class AR_REV_V_CURR_API AProjectile : public AActor
{
	GENERATED_BODY()
public:
	// Sets default values for this actor's properties
	AProjectile(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;


	// struct that holds all important properties that defines the projectiles behaviour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Projectile|Settings")
		FProjectileProperties ProjectileProperties;

	// Velocity that is being inherited from the weapon that is shooting the projectile (in world Space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Projectile|Movement")
		FVector AdditionalVelocity;

	// Current Velocity vector of the Projectile in world space
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		FVector Velocity;

	// if the velocity of the projectile is lower it will be destroyed, set to 0 to disable the destruction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		float MinVelocitySquarred;

	// actors to ignore while moving
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Projectile|Collision")
		TArray<AActor*> IgnoreActors;

	// 1.0 -> 100% Velocity after bounce, will be combined with the bounciness of the Physical Materials Restitution if it exists
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
		float Bounciness;

	// -1 unlimited bounces; 0 bouncing disabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
		int32 MaxBounces;

	// dot product of the velocity vector and the surface normal, choose in range of 1.0 (always bounce when bUsePhysicalMaterial deactivated) and 0.0 (never bounce)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
		float BounceThreshold;

	// use the physical material of the actor the projectile hits in order to calculate impact/bounce behaviour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Collision")
		bool bUsePhysicalMaterial;

	// Actors Locations: index 0 = StartLocation; last index = EndLocation; between 1st and last: all the locations where the actor bounced/penetrated
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		TArray<FVector> Locations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement|DEBUG")
		bool bCanMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement|DEBUG")
		float DEBUGLineLifeTime = 5.0f;


	FTransform DEBUGStart;

private:
	FVector TraceStartLocation;
	FVector TraceEndLocation;
	float PendingTravel;
	int32 Bounces;
	// will be calculated every time CanBounce is called
	float lastImpactDotProduct;
	bool bUnlimitedBouncing;
	bool bBounceAgain;
	bool bPendingDestruction;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Collsion")
		void OnBounce(const FHitResult& HitResult);

	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Collsion")
		void OnImpact(const FHitResult& HitResult);

	FORCEINLINE virtual void Movement();

	FORCEINLINE virtual void TraceAfterBounce();

	FORCEINLINE virtual bool BouncingAllowed();
	// calculates the dot product of the impact and decides whether the projectile can bounce
	FORCEINLINE virtual bool CanBounce(const FHitResult& Hit);

	virtual void Impact(const FHitResult& Hit);
	virtual void Bounce(const FHitResult& Hit);
	FORCEINLINE virtual void HandleTraceResult(const FHitResult& Hit);
	FORCEINLINE virtual void UpdateTransform();
};
