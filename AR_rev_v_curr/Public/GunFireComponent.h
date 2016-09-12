// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "GunFireComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AR_REV_V_CURR_API UGunFireComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGunFireComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void StartGunFire(const int32 AmmoAmount,
			const float BaseFireInterval,
			class UPrimitiveComponent* GunBarrel,
			const TArray<FName> & GunSockets,
			const int32 GunNumSalves,
			const int32 NumProjectiles,
			const float GunSalveDensity = 1.0f,
			const float WeaponSpreadHalfAngle = 0.5f,
			const float GunRecoilForce = 0.0f,
			const int32 TracerIntervall = 1)
	{
		StopGunFire();
		bGunFire = true;
		this->AmmoAmount = AmmoAmount;
		this->BaseFireInterval = BaseFireInterval;
		this->GunBarrel = GunBarrel;
		this->GunNumSalves = GunNumSalves;
		this->NumProjectiles = NumProjectiles;

		WeaponSpreadRadian = WeaponSpreadHalfAngle * PI / 180.0f;
		GunSalveIntervall = (GunSalveDensity * BaseFireInterval) / GunNumSalves;

		this->GunRecoilForce = GunRecoilForce;
		this->TracerIntervall = TracerIntervall;
		this->GunSockets = GunSockets;

		StartGunFire();
	}

	void StartGunFire() {

		if (bGunReady && AmmoAmount > 0)
		{
			// gun is ready to fire
			// make sure no other gunfire timer is activ by clearing it
			if (GetOwner()) {
				GetOwner()->GetWorldTimerManager().ClearTimer(GunFireHandle);
				// activate a new gunfire timer
				GetOwner()->GetWorldTimerManager().SetTimer(GunFireHandle, this, &UGunFireComponent::GunFire, BaseFireInterval, true, 0.0f);
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun ON");
			}
		}
		else if (GetOwner() && !GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireCooldown))
		{ // gun is not cooling down but could not be fired
		  // enable gun
			bGunReady = true;
			// debug
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun COOLDOWN NOT ACTIVATED");
			// try again to fire gun
			StartGunFire();
		}
		else
		{
			// gun is cooling down or out of ammo
			if (AmmoAmount < 1) {
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OUT OF AMMO");
				StopGunFire();
			}
			else if (GetOwner() && GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireCooldown))
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun STILL COOLING DOWN");
			}
		}
	}

	void GunFire()
	{
		// start only fireing if there is ammuntion to do so
		if (AmmoAmount > 0)
		{
			// reset the salve counter
			GunCurrentSalve = 0;
			// start a subtimer that fires GunNumSalves salves
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun SALVE");
			if (GetOwner()) GetOwner()->GetWorldTimerManager().SetTimer(GunSalveTimerHandle, this, &UGunFireComponent::GunFireSalve, GunSalveIntervall, true, 0.0f);
		}
	}

	void GunFireSalve()
	{
		// fire a salve only if not all salves have been fired
		if (GunBarrel && GunCurrentSalve < GunNumSalves)
		{

			// loop through the projectiles in each salve to be fired
			for (uint8 shot = 0; shot < NumProjectiles; ++shot)
			{
				// stop fireing if there is no ammunition
				if (AmmoAmount < 1)
				{
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "Gun OUT OF AMMO!");
					if (GetOwner()) GetOwner()->GetWorldTimerManager().ClearTimer(GunSalveTimerHandle);
					return;
				}

				// choose next avaliable gun sockets or start over from the first if last was used
				CurrGunSocketIndex = (CurrGunSocketIndex + 1) % GunSockets.Num();
				// get the tranform of the choosen socket

				const FTransform& CurrentSocketTransform = GunBarrel->GetSocketTransform(GunSockets[CurrGunSocketIndex]);

				FVector SpawnDirection = CurrentSocketTransform.GetRotation().GetForwardVector();

				// add weaponspread
				SpawnDirection = FMath::VRandCone(SpawnDirection, WeaponSpreadRadian);

				UWorld* const World = GetWorld();
				AActor* const Owner = GetOwner();

				// spawn/fire projectile with tracers
				if (TracerIntervall > 0)
				{
					// loop Tracer-Counter
					CurrentTracer = (CurrentTracer + 1) % TracerIntervall;

					// Spawn projectile, if Current-Tracer == 0 a tracer will be visible/spawned
					SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), CurrentTracer == 0, CurrentSocketTransform.GetLocation());
					
					if (World && ProjectileClass) {
						FActorSpawnParameters SpawnParams;
						SpawnParams.Owner = Owner;
						SpawnParams.Instigator = Owner ? Owner->GetInstigator() : nullptr;
						// spawn the projectile at the muzzle
						AActor* const Projectile = World->SpawnActor<AActor>(ProjectileClass, CurrentSocketTransform.GetLocation(), SpawnDirection.Rotation(), SpawnParams);
						//if (Projectile)
//						{
							// find launch direction
//							FVector const LaunchDir = MuzzleRotation.Vector();
//							Projectile->InitVelocity(LaunchDir);
//						}
					}
				}
				else
				{
					// spawn projectiles without tracers
					SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), false, CurrentSocketTransform.GetLocation());
				}
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun BANG");
				// decrease ammunition after each shot
				--AmmoAmount;

				// add recoil to rootcomponent
				if (GetOwner() && GetOwner()->GetRootComponent() && GetOwner()->GetRootComponent()->IsSimulatingPhysics()) {
					class UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
					if (Root) Root->AddImpulseAtLocation(SpawnDirection * GunRecoilForce * FMath::FRandRange(0.5f, 1.5f), CurrentSocketTransform.GetLocation());
				}
			}
			// increase salve counter
			++GunCurrentSalve;
		}
		else
		{
			// deactivate the salve-timer if all salves have been fired
			if (GetOwner()) GetOwner()->GetWorldTimerManager().ClearTimer(GunSalveTimerHandle);
		}
	}

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void StopGunFire()
	{
		// player has gunfire button released
		bGunFire = false;
		// is a gunfire timer active
		if (GetOwner() && GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireHandle))
		{
			// stop the timer
			GetOwner()->GetWorldTimerManager().PauseTimer(GunFireHandle);
			// make sure gun is disabled
			bGunReady = false;
			// store the remaining time
			const float CoolDownTime = GetOwner()->GetWorldTimerManager().GetTimerRemaining(GunFireHandle);
			// remove old gunfire timer
			GetOwner()->GetWorldTimerManager().ClearTimer(GunFireHandle);
			// create a new timer to reactivate gun after a cooldownperiod		
			GetOwner()->GetWorldTimerManager().SetTimer(GunFireCooldown, this, &UGunFireComponent::GunCooldownElapsed, CoolDownTime, false);
		}
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OFF");
	}

	void GunCooldownElapsed()
	{
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "Gun COOLED");
		// gun has cooled down and is again ready to fire
		bGunReady = true;
		// has the user requested fireing reactivate gunfire
		if (bGunFire)
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun CONTINUE");
			StartGunFire();
		}
	}



	UFUNCTION(BlueprintNativeEvent, Category = "GunFireComponent")
		void SpawnProjectile(const FTransform &SocketTransform, const bool bTracer, const FVector &FireBaseVelocity = FVector::ZeroVector, const FVector &TracerStartLocation = FVector::ZeroVector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent")
		TArray<FName> GunSockets;

	/** TODO: create CPP Projectile
	Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AActor> ProjectileClass;



private:
	class UPrimitiveComponent* GunBarrel;
	bool bGunFire;
	bool bGunReady;
	int32 AmmoAmount;
	float BaseFireInterval;
	float GunSalveIntervall;
	int32 GunCurrentSalve;
	int32 GunNumSalves;
	int32 NumProjectiles;
	int32 CurrGunSocketIndex;
	float WeaponSpreadRadian;
	float GunRecoilForce;
	FTimerHandle GunFireHandle;
	FTimerHandle GunFireCooldown;
	FTimerHandle GunSalveTimerHandle;
	int32 TracerIntervall = 1;
	int32 CurrentTracer;
};
