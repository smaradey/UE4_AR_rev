// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#define LOG_MSG 1
#include "CustomMacros.h"
#include "Explosive.generated.h"

UCLASS()
class AR_REV_V_CURR_API AExplosive : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AExplosive(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
	URadialForceComponent* RadialForce;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Radius in which other Explosives are activated/other actors can be damaged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
		float Range = { 500.0f };

	// Used to calculate the explosion delay of explosives (v in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive")
		float ShockWaveVelocity = { 30000.0f };

	UFUNCTION(BlueprintCallable, Category = "Explosive|Shockwave")
		void ReceiveShockwave(UObject* InstigatingObject, const float Delay);

	UFUNCTION(BlueprintCallable, Category = "Explosive|Detonation")
		void Detonate(const FTransform& Transform);


protected:
	// Timer that simulates traveltime of shockwaves
	FTimerHandle ShockWaveReceiveTimer;

	bool bExploded = { false };

private:
	FORCEINLINE void DelayedDetonation();

	// returns true if a timer is pending or is activ
	FORCEINLINE bool IsTimerActivByHandle(const FTimerHandle& Timer) const;



};
