// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

// Enum to define a Projectiles Type: Kinetic/Energy
UENUM(BlueprintType)
enum class EProjectileType : uint8
{
	// has a mass, can bounce, is affected by gravity
	Kinetic 	        UMETA(DisplayName = "Kinetic-Type"),
	// has no mass, can not bounce, is not affected by gravity
	Energy			 	UMETA(DisplayName = "Energy-Type")
};

// struct that holds all important properties that defines the projectiles behaviour
USTRUCT(BlueprintType)
struct FProjectileProperties {
	GENERATED_USTRUCT_BODY()

	// v in cm/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		float MuzzleVelocity;

	// maximum range in cm the projectile can fly
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		float MaxRange;

	// projectiles has a bright tracer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		bool bTracer;

	// color which the tracer emmits
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		FLinearColor TracerColor;

	// defines the type of projectile (kinetic/energy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		EProjectileType ProjectileType;

	// mass in kg of the Projectile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		float Mass;
};

UCLASS()
class AR_REV_V_CURR_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProjectile();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// struct that holds all important properties that defines the projectiles behaviour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Projectile|Settings")
		FProjectileProperties ProjectileProperties;

	void SetProjectileProperties(const FProjectileProperties& Properties)
	{
		ProjectileProperties = Properties;
	}



};
