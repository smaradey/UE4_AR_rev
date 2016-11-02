#pragma once
#include "Damage.generated.h"

USTRUCT(BlueprintType)
struct FBaseDamage {
	GENERATED_USTRUCT_BODY()
	
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		float MinDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Settings")
		float MaxDamage = 100.0f;
};

