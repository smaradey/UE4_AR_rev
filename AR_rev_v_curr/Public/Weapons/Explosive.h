// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#define LOG_MSG 1
#include "CustomMacros.h"
#include "Explosive_Interface.h"
#include "Damage.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive", meta = (ClampMin = "1.0", ClampMax = "10000000000.0", UIMin = "1.0", UIMax = "100000.0"))
		float Range = { 500.0f };

	// Used to calculate the explosion delay of explosives (v in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive", meta = (ClampMin = "0.1", ClampMax = "29979245800.0", UIMin = "0.1", UIMax = "29979245800.0"))
		float ShockWaveVelocity = { 30000.0f };

	// Damage that is being applied to actors in Range, randomly choosen between min/max
	// if Radialforce has option Falloff set to Linear: Damage drops of to zero the further actors are away from Center of Explosion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive", Meta = (ExposeOnSpawn = true))
	FBaseDamage Damage;

	// if true will perform check if there are no obstacles between explosion center and actors in range (wip)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosive|Damage")
		bool bDamagesOnlyActorsInLineOfSight = { false };

	// add actors or objects that want to be notified when this Actor detonates,
	// they need to implement the Interface "Explosive_Interface" in order to bind functionality to the explosion
	// if the list is empty: this actors ParentActor will be notified (the actor this actor is attached to, needs to implement the Interface)
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Explosive", Meta = (ExposeOnSpawn = true))
	TArray<UObject*> Observers;

	UFUNCTION(BlueprintCallable, Category = "Explosive|Shockwave")
		void ReceiveShockwave(UObject* InstigatingObject, const float Delay);

	// returns false if Explosive already detonated or Explosive is already pending explosion with a shorter Delay
	UFUNCTION(BlueprintCallable, Category = "Explosive|Detonation")
		bool DetonateAfter(const float Delay);

	UFUNCTION(BlueprintCallable, Category = "Explosive|Detonation")
		void Detonate(const FTransform& Transform);

	UFUNCTION(NetMulticast, Reliable)
		void Detonated(const FTransform& Transform);


protected:
	// Timer that simulates traveltime of shockwaves
	FTimerHandle ShockWaveReceiveTimer;

	bool bExploded = { false };

private:
	FORCEINLINE void DelayedDetonation();

	// returns true if a timer is pending or is activ
	FORCEINLINE bool IsTimerActivByHandle(const FTimerHandle& Timer) const;
};
