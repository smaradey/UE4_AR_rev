#pragma once

#include "Missile_Enums.generated.h"

UENUM(BlueprintType)
enum class EHomingType : uint8
{
	None 	        UMETA(DisplayName = "None"),
	// Follow Target Location
	Simple			UMETA(DisplayName = "Simple"),
	// Use Target Location Prediction
	Advanced		UMETA(DisplayName = "Advanced")
};