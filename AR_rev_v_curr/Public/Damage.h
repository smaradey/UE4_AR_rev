#pragma once
#include "Damage.generated.h"

USTRUCT(BlueprintType)
struct FBaseDamage {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		float MinDamage = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		float MaxDamage = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		TSubclassOf<class UDamageType> DamageTypeClass;
};

