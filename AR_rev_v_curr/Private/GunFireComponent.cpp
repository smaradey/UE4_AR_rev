// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "GunFireComponent.h"
#include "Gun_Interface.h"


// Sets default values for this component's properties
UGunFireComponent::UGunFireComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = true;

	// ...
}


// Called when the game starts
void UGunFireComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	Initialize(GunProperties);
}


// Called every frame
void UGunFireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateOwner();
	// ...
}

void UGunFireComponent::Initialize(const FGunProperties& Gun)
{
	this->GunProperties = Gun;

	GunSalveIntervall = (Gun.SalveDistributionInCycle * Gun.FireCycleInterval) / Gun.NumSalvesInCycle;

	// WeaponSpreadRadian = WeaponSpreadHalfAngle * PI / 180.0f;
	GunSalveIntervall = (Gun.SalveDistributionInCycle * Gun.FireCycleInterval) / Gun.NumSalvesInCycle;
}

void UGunFireComponent::StartFiring(class UPrimitiveComponent* Barrel, const TArray<FName>& MuzzleSockets)
{
	bGunFireRequested = true;

	// update GunBarrel and Sockets
	this->GunBarrel = Barrel;
	this->GunSockets = MuzzleSockets;

	if (mCurrentStatus == EGunStatus::Idle)
	{
		StartGunFire();
	}
	//
	//	switch (mCurrentStatus)
	//	{
	//	case EGunStatus::Deactivated: {}
	//								  break;
	//	case EGunStatus::Empty: {}
	//							break;
	//	case EGunStatus::Firing: {}
	//							  break;
	//	case EGunStatus::Idle: {
	//		StartGunFire();
	//	}
	//						   break;
	//	case EGunStatus::Overheated: {
	//		bGunFireRequested = true;
	//	}
	//								 break;
	//	case EGunStatus::Reloading: {
	//		bGunFireRequested = true;
	//	}
	//								break;
	//	default: {
	//
	//	}
	//	}
}

void UGunFireComponent::StartGunFire()
{
	if (GetOwner())
	{
		// make sure The GunFire
		GetOwner()->GetWorldTimerManager().ClearTimer(GunFireHandle);
		// change the Status to Firing
		mCurrentStatus = EGunStatus::Firing;
		// activate a new gunfire timer
		GetOwner()->GetWorldTimerManager().SetTimer(GunFireHandle, this, &UGunFireComponent::StartFiringCycle, GunProperties.FireCycleInterval, true, 0.0f);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Starting FireCycle");
	}
	else
	{
		// TODO: Error handling
		mCurrentStatus = EGunStatus::Error;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Error: StartGunFire: No Owner");
	}
}

void UGunFireComponent::StartFiringCycle()
{
	// start only Firing if there is ammuntion to do so
	if (GetOwner())
	{
		if (MagHasAmmo())
		{
			// reset the salve counter
			GunCurrentSalve = 0;
			// start a subtimer that fires GunNumSalves salves
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun CYCLE");
			GetOwner()->GetWorldTimerManager().SetTimer(GunSalveTimerHandle, this, &UGunFireComponent::GunFireSalve, GunSalveIntervall, true, 0.0f);
		}
		else
		{
			ReloadMagazine();
		}
	}
	else
	{
		// TODO: Error handling
		mCurrentStatus = EGunStatus::Error;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Error: StartFiringCycle: No Owner");
	}
}

void UGunFireComponent::GunFireSalve()
{
	// fire a salve only if not all salves have been fired
	if (GunBarrel && GunCurrentSalve < GunProperties.NumSalvesInCycle)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun SALVE");
		// loop through the projectiles in each salve to be fired
		for (uint8 shot = 0; shot < GunProperties.NumProjectilesInSalve; ++shot)
		{
			// stop Firing if there is no ammunition
			if (!MagHasAmmo())
			{
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "Magazine is empty!");
				ReloadMagazine();
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

			// loop Tracer-Counter
			CurrentTracer = (CurrentTracer + 1) % GunProperties.TracerOrder.Num();
			// spawn/fire projectile with tracers


			// Spawn projectile, if Current-Tracer == 0 a tracer will be visible/spawned
			//SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), CurrentTracer == 0, CurrentSocketTransform.GetLocation());

			if (World && ProjectileClass && GunBarrel)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = Owner;
				SpawnParams.Instigator = Owner ? Owner->GetInstigator() : nullptr;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				const FTransform& SpawnTransform = FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation());
				// spawn the projectile at the muzzle
				AProjectile* const Projectile = World->SpawnActor<AProjectile>(ProjectileClass, CurrentSocketTransform.GetLocation(), SpawnDirection.Rotation(), SpawnParams);
				int32 ProjectileTypeIndex = 0;
				if (Projectile && GunProperties.ProjectileProperties.IsValidIndex(ProjectileTypeIndex))
				{
					FProjectileProperties& ProjectileProperties = GunProperties.ProjectileProperties[ProjectileTypeIndex];
					ProjectileProperties.bTracer = GunProperties.TracerOrder.Num() > 0 ? GunProperties.TracerOrder[CurrentTracer] : true;
					Projectile->SetProjectileProperties(ProjectileProperties);

					// Call the parents Interface Function to notify it of a spawn
					if (Owner && Owner->GetClass()->ImplementsInterface(UGun_Interface::StaticClass()))
					{
						IGun_Interface::Execute_ProjectileSpawned(Owner, ProjectileProperties, SpawnTransform);
					}
				}
			}

			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun BANG");
			// decrease ammunition after each shot
			GunProperties.CurrentMagazineLoad--;

			// add recoil to rootcomponent
			if (GetOwner() && GetOwner()->GetRootComponent() && GetOwner()->GetRootComponent()->IsSimulatingPhysics())
			{
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

void UGunFireComponent::StopGunFire()
{
	// player has gunfire button released
	bGunFireRequested = false;
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
		GetOwner()->GetWorldTimerManager().SetTimer(GunFireCycleCooldown, this, &UGunFireComponent::GunCooldownElapsed, CoolDownTime, false);
	}
	// debug
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OFF");
}

void UGunFireComponent::ReloadMagazine()
{
	if (GetOwner()) GetOwner()->GetWorldTimerManager().ClearTimer(GunSalveTimerHandle);

	if (IsOutOfAmmo())
	{
		if (MagHasAmmo()) return;
		mCurrentStatus = EGunStatus::Empty;
		return;
	}

	if (GetOwner())
	{
		// get the remaining Cycle Time
		const float CycleCoolDown = GetOwner()->GetWorldTimerManager().GetTimerRemaining(GunFireHandle);
		// stop the Cycle Timer
		GetOwner()->GetWorldTimerManager().ClearTimer(GunFireHandle);
		// change the status
		mCurrentStatus = EGunStatus::Reloading;
		// Start a timer for the Cylce cooldown
		GetOwner()->GetWorldTimerManager().SetTimer(GunFireCycleCooldown, this, &UGunFireComponent::GunCooldownElapsed, CycleCoolDown, false);

		// the reloading:
		const int32 MagRemaining = GunProperties.CurrentMagazineLoad;
		int32 NumProjectilesToAdd = GunProperties.MagazineSize - MagRemaining;

		// check if Mag is 100% empty
		if (MagRemaining == 0)
		{
			// decrease Number of Projectiles to add to Mag by One
			NumProjectilesToAdd--;
		}
		// take Projectiles from available
		int32 ResultTotalAmmo = GunProperties.TotalAmmunitionCount - NumProjectilesToAdd;

		// check if there were more taken than available
		if (ResultTotalAmmo < 0)
		{
			// reduce the number of projectiles that can be added to the Magazine
			NumProjectilesToAdd += ResultTotalAmmo;
			GunProperties.TotalAmmunitionCount = 0;
		}
		else
		{
			GunProperties.TotalAmmunitionCount = ResultTotalAmmo;
		}

		// Fill the Mag
		GunProperties.CurrentMagazineLoad += NumProjectilesToAdd;

		GetOwner()->GetWorldTimerManager().SetTimer(GunReloadCooldown, this, &UGunFireComponent::ReloadingFinished, GunProperties.ReloadTimeWholeMagazine, false);
	}
	else
	{
		// TODO: Error handling
		mCurrentStatus = EGunStatus::Error;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Error: Reloading: No Owner");
	}
}

void UGunFireComponent::GunCooldownElapsed()
{
	bool bIsFiring = false;
	if (GetOwner())
	{
		// if the gun cycle is not over or the gun is overheated set the status to "Overheated"
		if (GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireOverheatingCooldown))
		{
			mCurrentStatus = EGunStatus::Overheated;
			return;
		}

		bIsFiring = GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireHandle);
	}
	else
	{
		// TODO: Error handling
		mCurrentStatus = EGunStatus::Error;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Error: ReloadingFinished: No Owner");
	}

	if (bIsFiring)
	{
		return;
	}
	if (bGunFireRequested)
	{
		StartGunFire();
		return;
	}

	// TODO: potential bug when gun is being deactivated while it is reloading
	mCurrentStatus = EGunStatus::Idle;
}

void UGunFireComponent::ReloadingFinished()
{
	if (GetOwner())
	{
		// if the gun cycle is not over or the gun is overheated set the status to "Overheated"
		if (GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireCycleCooldown) || GetOwner()->GetWorldTimerManager().IsTimerActive(GunFireOverheatingCooldown))
		{
			mCurrentStatus = EGunStatus::Overheated;
			return;
		}
	}
	else
	{
		// TODO: Error handling
		mCurrentStatus = EGunStatus::Error;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "Error: ReloadingFinished: No Owner");
	}


	if (bGunFireRequested)
	{
		StartGunFire();
		return;
	}
	// TODO: potential bug when gun is being deactivated while it is reloading
	mCurrentStatus = EGunStatus::Idle;
}

void UGunFireComponent::UpdateOwner() const
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<UGun_Interface>())
	{
		FWeaponStatus Status;
		Status.Status = mCurrentStatus;
		IGun_Interface::Execute_WeaponStatusChanged(Owner, Status);
	}
}

bool UGunFireComponent::MagHasAmmo() const
{
	return GunProperties.CurrentMagazineLoad > 0;
}

bool UGunFireComponent::IsOutOfAmmo() const
{
	return GunProperties.TotalAmmunitionCount < 1;
}

void UGunFireComponent::SpawnProjectile_Implementation(const FTransform& SocketTransform, const bool bTracer, const FVector& FireBaseVelocity, const FVector& TracerStartLocation)
{
	// method overridden by blueprint to spawn the projectile
}
