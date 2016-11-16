// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Damage.h"
#include "Projectile_Enums.h"
#include "Projectile_Structs.h"
#define LOG_MSG 1
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

	// 1.0 -> 100% Velocity after bounce
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		float Bounciness;

	// -1 unlimited bounces; 0 bouncing disabled
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		int32 MaxBounces;

	// dot product of the velocity vector and the surface normal, choose in range of -1.0 (always bounce) and 0.0 (never bounce)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
		float BounceThreshold;




private:
	FVector TraceStartLocation;
	FVector TraceEndLocation;
	float PendingTravel;
	int32 Bounces;
	bool bUnlimitedBouncing;
	bool bBounceAgain;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Collsion")
		void OnBounce();

	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Collsion")
		void OnImpact();

	virtual void Movement();

	virtual void TraceAfterBounce();

	virtual FORCEINLINE bool BouncingAllowed();
	virtual FORCEINLINE bool CanBounce(const FHitResult& Hit);

	virtual void Impact(const FHitResult& Hit);
	virtual void Bounce(const FHitResult& Hit);
	virtual void HandleTraceResult(const FHitResult& Hit);
	virtual void UpdateTransform();
};
