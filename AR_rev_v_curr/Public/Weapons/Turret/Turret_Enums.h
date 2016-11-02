#pragma once

#include "Turret_Enums.generated.h"

UENUM(BlueprintType)
enum class ETurretOperationMode : uint8
{
	Rest		UMETA(DisplayName = "Rest"),
	Track 		UMETA(DisplayName = "Track"),
	AimOnce	    UMETA(DisplayName = "AimOnce"),
	Freeze		UMETA(DisplayName = "Freeze")
};

UENUM(BlueprintType)
enum class ETurretRotationOrder : uint8
{
	Simultaneously		UMETA(DisplayName = "Simultaneously"),
	YawPitch 			UMETA(DisplayName = "YawPitch"),
	PitchYaw			UMETA(DisplayName = "PitchYaw")
};