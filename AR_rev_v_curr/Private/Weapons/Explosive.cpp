// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Explosive.h"


// Sets default values
AExplosive::AExplosive(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	SetActorEnableCollision(false);
	bReplicates = true;

	RadialForce = ObjectInitializer.CreateDefaultSubobject<URadialForceComponent>(this, TEXT("DetonationImpuls"));
	RadialForce->SetActive(false);
	RadialForce->Radius = Range;
	RadialForce->bAutoActivate = false;
	RadialForce->Falloff = ERadialImpulseFalloff::RIF_Linear;
	RadialForce->ImpulseStrength = 100000.0f;
	RadialForce->DestructibleDamage = 10000000000.0f;
}


void AExplosive::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AExplosive, Var);
}

void AExplosive::ReceiveShockwave(UObject* InstigatingObject, const float Delay)
{
	if (bExploded) return;
	if (InstigatingObject)
	{
		LOGA2("Explosive: Received Explosion Command from \"%s\"; Explodes in %f seconds", *InstigatingObject->GetName(), Delay)
	}
	if (Delay > 0.0f)
	{
		FTimerManager& TimerManager = GetWorldTimerManager();
		if (!IsTimerActivByHandle(ShockWaveReceiveTimer)
			|| TimerManager.GetTimerRemaining(ShockWaveReceiveTimer) > Delay)
		{
			TimerManager.SetTimer(ShockWaveReceiveTimer, this, &AExplosive::DelayedDetonation, Delay, false);
		}
		return;
	}
	Detonate(GetActorTransform());
}

void AExplosive::Detonate(const FTransform& Transform)
{
	if (bExploded || !HasAuthority()) return;
	bExploded = true;

	//TODO: Replicate Explosion to all clients
	TSubclassOf<AActor> Class = AActor::GetClass();
	int32 num = 0;
	UWorld* World = GetWorld();
	if (World) {

		if (RadialForce) RadialForce->FireImpulse();

		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor != nullptr && Actor != this)
			{
				const float DistToActor = (GetActorLocation() - Actor->GetActorLocation()).Size();
				if (DistToActor > Range) continue;

				// TODO: check line of sight

				LOGA2("AExplosive: In Explosion Radius No: %d: \"%s\"", ++num, *Actor->GetName());

				AExplosive* ExplosiveActor = Cast<AExplosive>(Actor);
				// Trigger other explosives in Range
				// TODO: change triggering to damage based system
				if (ExplosiveActor != nullptr && !ExplosiveActor->bExploded)
				{
					const float ShockwaveReceiveDelay = DistToActor / ShockWaveVelocity;
					LOGA2("AExplosive: \"%s\" is child class of AExplosive and will explode in %lf seconds", *Actor->GetName(), ShockwaveReceiveDelay)
						ExplosiveActor->ReceiveShockwave(this, ShockwaveReceiveDelay);
					continue;
				}

				// TODO: Damage other actors
			}
		}
	}
	Destroy();
}

void AExplosive::DelayedDetonation()
{
	LOGA("Explosive: \"%s\": Delayed Detonation executed", *GetName())
		Detonate(GetActorTransform());
}

bool AExplosive::IsTimerActivByHandle(const FTimerHandle& Timer) const
{
	FTimerManager& WorldTimerManager = GetWorldTimerManager();
	return WorldTimerManager.IsTimerPending(Timer) || WorldTimerManager.IsTimerActive(Timer);
}


