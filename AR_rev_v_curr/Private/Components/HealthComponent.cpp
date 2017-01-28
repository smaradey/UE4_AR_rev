// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "HealthComponent.h"


// Sets default values for this component's properties
UHealthComponent::UHealthComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	SetIsReplicated(true);
	SetComponentTickInterval(1.0f);
	bAutoActivate = true;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if(GetOwnerRole() != ROLE_Authority)
	{
		SetComponentTickEnabled(false);
	}
	// ...
}


// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// subtract the repair-/recharge delay from the DeltaTime after Tick has been reenabled
	{
		const float CurrentGameTime = { GetGameTime() };
		if (CurrentGameTime > 0.0f && TimeTickEnabled > 0.0f)
		{
			const float TimeSinceTickActivation = { CurrentGameTime - TimeTickEnabled };
			const float ActivationDeltaTime = { DeltaTime - TimeSinceTickActivation };
			if (ActivationDeltaTime > 0.0f)
			{
				DeltaTime = ActivationDeltaTime;
			}
		}
	}

	bool IsCompletelyChargedRepaired = { true };
	if (CanRepairHull())
	{
		IsCompletelyChargedRepaired = false;
		HullCurrent = FMath::FInterpConstantTo(HullCurrent, HullMax, HullRepairSpeed, DeltaTime);
	}
	if (CanRechargeShield())
	{
		IsCompletelyChargedRepaired = false;
		ShieldCurrent = FMath::FInterpConstantTo(ShieldCurrent, ShieldMax, ShieldRechargeSpeed, DeltaTime);
	}
	if (IsCompletelyChargedRepaired)
	{
		SetComponentTickEnabled(false);
	}
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, ShieldMax);
	DOREPLIFETIME(UHealthComponent, ShieldCurrent);
	DOREPLIFETIME(UHealthComponent, HullMax);
	DOREPLIFETIME(UHealthComponent, HullCurrent);
	DOREPLIFETIME(UHealthComponent, bRepairAllowed);
	DOREPLIFETIME(UHealthComponent, bRechargeAllowed);
}

bool UHealthComponent::CanRechargeShield() const
{
	return bRechargeAllowed && ShieldRechargeSpeed != 0.0f && ShieldMax > 0.0f && ShieldMax > ShieldCurrent && !IsTimerActivByHandle(ShieldRechargeDelayTimer);
}

bool UHealthComponent::CanRepairHull() const
{
	return bRepairAllowed && HullRepairSpeed != 0.0f && HullMax > 0.0f && HullMax > HullCurrent && !IsTimerActivByHandle(HullRepairDelayTimer);
}

bool UHealthComponent::ShieldTakeInstantDamage(const float DamageAmount, const bool CanDamageHull)
{
	if (DamageAmount > 0.0f)
	{
		ActivateShieldRechargeDelay(ShieldRechargeDelay);
	}
	const float OldShield = { ShieldCurrent };
	ShieldCurrent = FMath::Max(0.0f, ShieldCurrent - DamageAmount);
	if (CanDamageHull && ShieldCurrent == 0.0f)
	{
		return HullTakeInstantDamage(DamageAmount - OldShield);
	}
	return true;
}

bool UHealthComponent::HullTakeInstantDamage(const float DamageAmount)
{
	if (DamageAmount > 0.0f)
	{
		ActivateHullRepairDelay(HullRepairDelay);
	}
	HullCurrent = FMath::Max(0.0f, HullCurrent - DamageAmount);
	return HullCurrent != 0.0f;
}

void UHealthComponent::ActivateShieldRechargeDelay(const float Delay)
{
	UWorld* const World{ GetWorld() };
	if (World)
	{
		World->GetTimerManager().SetTimer(ShieldRechargeDelayTimer, this, &UHealthComponent::ShieldRechargeDelayFinished, Delay);
	}
}

void UHealthComponent::ActivateHullRepairDelay(const float Delay)
{
	UWorld* const World{ GetWorld() };
	if (World)
	{
		World->GetTimerManager().SetTimer(HullRepairDelayTimer, this, &UHealthComponent::HullRepairDelayFinished, Delay);
	}
}

void UHealthComponent::ShieldRechargeDelayFinished_Implementation()
{
	EnableTick();
}

void UHealthComponent::HullRepairDelayFinished_Implementation()
{
	EnableTick();
}

bool UHealthComponent::IsTimerActivByHandle(const FTimerHandle& Timer) const
{
	const UWorld* const World{ GetWorld() };
	return World && (World->GetTimerManager().IsTimerPending(Timer) || World->GetTimerManager().IsTimerActive(Timer));
}

float UHealthComponent::GetGameTime() const
{
	UWorld* const World{ GetWorld() };
	if (World)
	{
		return World->TimeSeconds;
	}
	return -1.0f;
}

void UHealthComponent::EnableTick()
{
	SetComponentTickEnabled(true);
	TimeTickEnabled = GetGameTime();
}
