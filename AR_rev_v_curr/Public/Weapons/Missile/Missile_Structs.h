#pragma once
#include "Missile_Enums.h"
#include "Damage.h"
#include "Missile_Structs.generated.h"

USTRUCT(BlueprintType)
struct FMissileStatus {
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Status")
		AActor* Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Status")
		float RemainingBoostDistance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Status")
		bool bIsHoming = false;
};

USTRUCT(BlueprintType)
struct FHomingProperties {
	GENERATED_USTRUCT_BODY()

		// Homing type
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		EHomingType Homing = EHomingType::Simple;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		float HomingActivationDelay = 0.2f;

	// if the target is not inside the missiles tracking angle it stops homing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		bool bCanLooseTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		float MaxTargetTrackingAngle = 90.0f;

	// Turnrate in deg/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		float MaxTurnrate = 110.0f;

	// advanced homing always working at full strength
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		bool bAlwaysActiv = false;

	// Distance to the target where Homing turns from simple into advanced
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		float ActivationDistance = 50000.0f;

	// Distance to the target where advanced Homing is working at full strength
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Homing")
		float DistanceCompletelyActive = 10000.0f;
};


// struct that holds all important properties that defines the missiles behaviour
USTRUCT(BlueprintType)
struct FMissileProperties {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties|Damage")
		FBaseDamage BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties|Damage")
		float ExplosionRadius = 50.0f;

	// v in cm/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties")
		float MaxVelocity = 30000.0f;

	// maximum range in cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties")
		float MaxRange = 400000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties")
		float AccelerationTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties|Homing")
		FHomingProperties HomingProperties;

	// Angle that the player uses to lock on to a target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile|Properties|Radar")
		float LockOnHalfAngle = 6.0f;
};
