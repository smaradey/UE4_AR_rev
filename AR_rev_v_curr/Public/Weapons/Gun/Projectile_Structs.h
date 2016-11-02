#pragma once
#include "Projectile_Enums.h"
#include "Projectile_Structs.generated.h"

// struct that holds all important properties that defines the projectiles behaviour
USTRUCT(BlueprintType)
struct FProjectileProperties {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		FBaseDamage BaseDamage;

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