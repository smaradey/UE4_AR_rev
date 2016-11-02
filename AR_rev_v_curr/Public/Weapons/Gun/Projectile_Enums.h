#pragma once

#include "Projectile_Enums.generated.h"

// Enum to define a Projectiles Type: Kinetic/Energy
UENUM(BlueprintType)
enum class EProjectileType : uint8
{
	// has a mass, can bounce, is affected by gravity
	Kinetic 	        UMETA(DisplayName = "Kinetic-Type"),
	// has no mass, can not bounce, is not affected by gravity
	Energy			 	UMETA(DisplayName = "Energy-Type")
};