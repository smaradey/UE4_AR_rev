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
	RootComponent = RadialForce;
}


void AExplosive::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AExplosive, Observers);
}

void AExplosive::ReceiveShockwave(UObject* InstigatingObject, const float Delay)
{
	if (InstigatingObject)
	{
		LOGA2("Explosive: Received Explosion Command from \"%s\"; Explodes in %f seconds", *InstigatingObject->GetName(), Delay)
	}
	DetonateAfter(Delay);
}

bool AExplosive::DetonateAfter(const float Delay)
{
	if (bExploded) return false;
	if (Delay > 0.0f)
	{
		FTimerManager& TimerManager = GetWorldTimerManager();
		if (!IsTimerActivByHandle(ShockWaveReceiveTimer)
			|| TimerManager.GetTimerRemaining(ShockWaveReceiveTimer) > Delay)
		{
			TimerManager.SetTimer(ShockWaveReceiveTimer, this, &AExplosive::DelayedDetonation, Delay, false);
			return true;
		}
		return false;
	}
	Detonate(GetActorTransform());
	return true;
}

void AExplosive::Detonate(const FTransform& Transform)
{
	if (bExploded || !HasAuthority()) return;
	bExploded = true;

	//TODO: Replicate Explosion to all clients
	Detonated(Transform);

	int32 num = 0;
	int32 totalNum = 0;
	UWorld* World = GetWorld();
	if (World) {

		bool bDamageDropOff = { false };
		if (RadialForce) {
			bDamageDropOff = RadialForce->Falloff == RIF_Linear;
			RadialForce->FireImpulse();
		}

		for (TActorIterator<AActor> It(World, AActor::StaticClass()); It; ++It)
		{
			AActor* Actor = *It;
			LOGA("AExplosive: Actor: \"%s\"", *Actor->GetName());
			++totalNum;
			if (Actor != nullptr && Actor != this)
			{
				const float DistToActor = (GetActorLocation() - Actor->GetActorLocation()).Size();
				if (DistToActor > Range)
				{
					//LOGA("AExplosive: Not in Explosion Radius No: \"%s\"", *Actor->GetName());
					continue;
				}
				//LOGA2("AExplosive: In Explosion Radius No: %d: \"%s\"", ++num, *Actor->GetName());

				// TODO: check line of sight

				// Trigger other explosives in Range
				AExplosive* ExplosiveActor = Cast<AExplosive>(Actor);

				// TODO: change triggering to damage based system
				if (ExplosiveActor != nullptr && !ExplosiveActor->bExploded)
				{
					const float ShockwaveReceiveDelay = DistToActor / ShockWaveVelocity;
					LOGA2("AExplosive: \"%s\" is child class of AExplosive and will explode in %lf seconds", *Actor->GetName(), ShockwaveReceiveDelay)
						ExplosiveActor->ReceiveShockwave(this, ShockwaveReceiveDelay);
					//continue;
				}

				if (Actor->bCanBeDamaged)
				{
					FDamageEvent DmgEvent;
					DmgEvent.DamageTypeClass = Damage.DamageTypeClass;
					float DamageToApply = FMath::RandRange(Damage.MinDamage, Damage.MaxDamage);
					if (bDamageDropOff) {
						const float RelativeDistanceToCenter = DistToActor / Range;
						DamageToApply *= RelativeDistanceToCenter;
					}
					LOGA2("AExplosive: Damaging \"%s\": Damageamount = %f", *Actor->GetName(), DamageToApply);
					Actor->TakeDamage(DamageToApply, DmgEvent, GetInstigatorController(), this);
				}
				//else
				//{
				//	LOGA("AExplosive: \"%s\" can not be damaged", *Actor->GetName());
				//}
			}
		}
		LOGA("AExplosive: Total Actors = %d", totalNum);
	}
	Destroy();
}

void AExplosive::Detonated_Implementation(const FTransform& Transform)
{
	SetActorTransform(Transform);
	if (RadialForce) {
		RadialForce->FireImpulse();
		LOGA("Explosive: \"%s\": Client Received Detonation", *GetName())
	}
	int32 Count = { 0 };
	for (UObject* Observer : Observers)
	{
		if (Observer) {
			if (Observer->GetClass()->ImplementsInterface(UExplosive_Interface::StaticClass()))
			{
				++Count;
				IExplosive_Interface::Execute_Detonated(Observer, this, Transform);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Explosive: Explosive_Interface not implemented by Observer: \"%s\""), *Observer->GetName());
			}
		}
	}
	if (Count == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Explosive: No Observers added or Explosive_Interface not implemented"));
	}
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


