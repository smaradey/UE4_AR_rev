// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))

class AR_REV_V_CURR_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UHealthComponent(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Shield")
		float ShieldMax = { 100.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Shield")
		float ShieldCurrent = { 100.0f };

	// shield recharge speed (unit/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Shield")
		float ShieldRechargeSpeed = { 10.0f };

	// amount of time recharging is not possible after absorbing damage with the shield
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Shield")
		float ShieldRechargeDelay = { 3.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Hull")
		float HullMax = { 100.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Hull")
		float HullCurrent = { 100.0f };

	// hull repair speed (unit/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Hull")
		float HullRepairSpeed = { 1.0f };

	// amount of time repair is not possible after taking damage to the hull
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HealthComponent|Hull")
		float HullRepairDelay = { 10.0f };

	// returns true if the timer with the delay is not activ
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HealthComponent|Shield")
		FORCEINLINE bool CanRechargeShield() const;

	// returns true if the timer with the delay is not activ
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "HealthComponent|Hull")
		FORCEINLINE bool CanRepairHull() const;

	// reduce ShieldCharge by DamageAmount, if CanDamageHull is true: damage Hull if Shield can not fully absorb the damage, returns true if Hull is not zero
	UFUNCTION(BlueprintCallable, Category = "HealthComponent|Shield")
		FORCEINLINE bool ShieldTakeInstantDamage(const float DamageAmount, const bool CanDamageHull = true);

	// reduce Hullpoints by DamageAmount, returns true if Hull is not zero
	UFUNCTION(BlueprintCallable, Category = "HealthComponent|Hull")
		FORCEINLINE bool HullTakeInstantDamage(const float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "HealthComponent|Shield")
		void ActivateShieldRechargeDelay(const float Delay);

	UFUNCTION(BlueprintCallable, Category = "HealthComponent|Hull")
		void ActivateHullRepairDelay(const float Delay);

	// make sure to call parent implementation as well in to make sure recharging/repairing is working
	UFUNCTION(BlueprintNativeEvent, Category = "HealthComponent|Shield")
	void ShieldRechargeDelayFinished();

	// make sure to call parent implementation as well in to make sure recharging/repairing is working
	UFUNCTION(BlueprintNativeEvent, Category = "HealthComponent|Hull")
	void HullRepairDelayFinished();


private:
	FTimerHandle ShieldRechargeDelayTimer = {};
	FTimerHandle HullRepairDelayTimer = {};
	float TimeTickEnabled;
	FORCEINLINE bool IsTimerActivByHandle(const FTimerHandle& Timer) const;

	// get the current game time in seconds, returns -1 in case of error
	FORCEINLINE float GetGameTime() const;
	FORCEINLINE void EnableTick();
};
