// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "ReservoirComponent.h"


// Sets default values for this component's properties
UReservoirComponent::UReservoirComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	SetIsReplicated(true);
	SetComponentTickInterval(InitialTickInterval);
	InitialTickInterval = GetComponentTickInterval();
	bAutoActivate = true;
}


// Called when the game starts
void UReservoirComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() != ROLE_Authority)
	{
		SetComponentTickEnabled(false);
	}
}


// Called every frame
void UReservoirComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// subtract the repair-/recharge delay from the DeltaTime after Tick has been reenabled
	const float CurrentGameTime = { GetGameTime() };
	if (CurrentGameTime > 0.0f && TimeTickEnabled > 0.0f)
	{
		const float TimeSinceTickActivation = { CurrentGameTime - TimeTickEnabled };
		if (TimeSinceTickActivation - DeltaTime < 0.0f)
		{
			DeltaTime = TimeSinceTickActivation;
		}
	}

	if (CanAutoRefill())
	{
		CurrentValue = FMath::FInterpConstantTo(CurrentValue, MaxValue, RefillSpeed, DeltaTime);
	}
	else {
		SetComponentTickEnabled(false);
	}
}

void UReservoirComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UReservoirComponent, MinValue);
	DOREPLIFETIME(UReservoirComponent, MaxValue);
	DOREPLIFETIME(UReservoirComponent, CurrentValue);
	DOREPLIFETIME(UReservoirComponent, bRefillAllowed);
}

bool UReservoirComponent::CanAutoRefill() const
{
	return bRefillAllowed && RefillSpeed != 0.0f && MaxValue != MinValue && MaxValue > CurrentValue && !IsTimerActivByHandle(RefillDelayTimer);
}

bool UReservoirComponent::ReduceByInstantly(const float Reduction, float& NewValue, float& Remaining, const bool bChangeStopsAutoRefill)
{
	if (Reduction != 0.0f)
	{
		if (bChangeStopsAutoRefill)
		{
			ActivateRefillDelay(RefillDelay);
		}
		const float RawResult = CurrentValue - Reduction;
		if (RawResult < MinValue)
		{
			Remaining = -RawResult;
			CurrentValue = MinValue;
		}
		else if (RawResult > MaxValue)
		{
			Remaining = -(RawResult - MaxValue);
			CurrentValue = MaxValue;
		}
		else
		{
			Remaining = 0.0f;
			CurrentValue = RawResult;
		}
	}
	NewValue = CurrentValue;
	return CurrentValue != MinValue;
}

bool UReservoirComponent::IncreaseByInstantly(const float Increase, float& NewValue, float& Remaining, const bool bChangeStopsAutoRefill)
{
	const bool Result = ReduceByInstantly(-Increase, NewValue, Remaining, bChangeStopsAutoRefill);
	Remaining = -Remaining;
	return Result;
}

void UReservoirComponent::ActivateRefillDelay(const float Delay)
{
	UWorld* const World = { GetWorld() };
	if (World && Delay > 0)
	{
		World->GetTimerManager().SetTimer(RefillDelayTimer, this, &UReservoirComponent::RefillDelayFinished, Delay);
	} else
	{
		EnableTick();
	}
}

void UReservoirComponent::RefillDelayFinished_Implementation()
{
	EnableTick();
}

bool UReservoirComponent::IsTimerActivByHandle(const FTimerHandle& Timer) const
{
	const UWorld* const World = { GetWorld() };
	return World && (World->GetTimerManager().IsTimerPending(Timer) || World->GetTimerManager().IsTimerActive(Timer));
}

float UReservoirComponent::GetGameTime() const
{
	UWorld* const World = { GetWorld() };
	if (World)
	{
		return World->TimeSeconds;
	}
	return -1.0f;
}

void UReservoirComponent::EnableTick()
{
	TimeTickEnabled = GetGameTime();
	SetComponentTickEnabled(true);
}

