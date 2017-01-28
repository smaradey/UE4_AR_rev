// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "ReservoirComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))

class AR_REV_V_CURR_API UReservoirComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	UReservoirComponent(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// default TickInterval, changing this value during runtime does NOT change the actual TickFrequency, use SetComponentTickInterval instead
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Tick")
		float InitialTickInterval = { 1.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Value")
		float MinValue = { 0.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Value")
		float MaxValue = { 100.0f };

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Value")
		float CurrentValue = { 100.0f };

	// shield recharge speed (unit/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Filling")
		float RefillSpeed = { 10.0f };

	// amount of time recharging is not possible after absorbing damage with the shield
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Filling")
		float RefillDelay = { 3.0f };

	// returns true if the timer with the delay is not activ
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ReservoirComponent|Filling")
		FORCEINLINE bool CanAutoRefill() const;

	// reduce Value by Reduction
	// -	returns true if Value is not MinValue
	// -	NewValue will contain the new Value (clamped to Min-/MaxValue)
	// -	Remaining will contain the amount that could not be removed from the FillValue
	//		example 1: Min = 0, Max = 100, Current = 30, Reduction = 50:
	//		NewValue = 0, Remaining = 20
	//		example 2: Min = 0, Max = 100, Current = 30, Reduction = -100:
	//		NewValue = 100, Remaining = -30
	UFUNCTION(BlueprintCallable, Category = "ReservoirComponent|Reducing")
		FORCEINLINE bool ReduceByInstantly(const float Reduction, float& NewValue, float& Remaining, const bool bChangeStopsAutoRefill = true);

	// increase Value by Increase
	// -	returns true if Value is not MinValue
	// -	NewValue will contain the new Value (clamped to Min-/MaxValue)
	// -	Remaining will contain the amount that could not be added to the FillValue
	//		example 1: Min = 0, Max = 100, Current = 70, Increase = 50:
	//		NewValue = 100, Remaining = 20
	//		example 2: Min = 0, Max = 100, Current = 30, Increase = -100:
	//		NewValue = 0, Remaining = -70
	UFUNCTION(BlueprintCallable, Category = "ReservoirComponent|Reducing")
		FORCEINLINE bool IncreaseByInstantly(const float Increase, float& NewValue, float& Remaining, const bool bChangeStopsAutoRefill = true);

	UFUNCTION(BlueprintCallable, Category = "ReservoirComponent|Filling")
		void ActivateRefillDelay(const float Delay);

	// make sure to call parent implementation as well in to make sure recharging/repairing is working
	UFUNCTION(BlueprintNativeEvent, Category = "ReservoirComponent|Filling")
		void RefillDelayFinished();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "ReservoirComponent|Filling")
		bool bRefillAllowed = { true };

protected:
	FTimerHandle RefillDelayTimer = {};

	// returns true if a timer is pending or is activ
	FORCEINLINE bool IsTimerActivByHandle(const FTimerHandle& Timer) const;

	// get the current game time in seconds, returns -1 in case of error
	FORCEINLINE float GetGameTime() const;

	float TimeTickEnabled;
	// enables Tick and stores the time of last call in TimeTickEnabled
	FORCEINLINE void EnableTick();
};
