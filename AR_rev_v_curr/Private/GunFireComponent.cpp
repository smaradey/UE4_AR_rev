// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "GunFireComponent.h"
#include "Gun_Interface.h"

#define DEBUG_MSG 1


// Sets default values for this component's properties
UGunFireComponent::UGunFireComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
	bReplicates = true;
	bAutoActivate = true;

	bGunChanged = true;

	// ...
}


// Called when the game starts
void UGunFireComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UGunFireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateOwner();
	// ...
}

void UGunFireComponent::Call_StartFiring(const FRandomStream& Stream)
{
	mRandomStream = Stream;
	if (bGunChanged)
	{
		Initialize();
	}
	bFiringRequested = bUseCycleCooldown = true;

	switch (mCurrentStatus)
	{
	case EGunStatus::Idle:
		{
			StartGunFire();
		}
		break;
	case EGunStatus::Deactivated: break;
	case EGunStatus::Firing: break;
	case EGunStatus::Empty: break;
	case EGunStatus::Reloading:
		{
			CancelReload();
		}
		break;
	case EGunStatus::Overheated: break;
	case EGunStatus::FinishingCycle: break;
	case EGunStatus::Error: break;
	default: break;
	}
}

void UGunFireComponent::Initialize()
{
	const FSpreadProperties& Spread = mGunProperties.SpreadProperties;

	// init Recoil ResetFactor
	const float& RecoilReturnTime = Spread.RecoilReturnTime;
	mRecoilResetFactor = 1.0f / FMath::Max(RecoilReturnTime, 0.001f);

	// precalculate Recoil
	mLocalRecoilDirections.Empty();
	mFPS_RecoilDeltaRotations.Empty();

	for (int i = 0; i < Spread.RecoilOffsetDirections.Num(); ++i)
	{
		if(Spread.InitialRecoil == 0.0f)
		{
			mLocalRecoilDirections.Add(FVector(1.0f, 0.0f, 0.0f));
			mFPS_RecoilDeltaRotations.Add(FVector2D::ZeroVector);
		}else
		{

			FVector SpreadStrength = FVector(1.0f, 0, 0).RotateAngleAxis(Spread.InitialRecoil, FVector(0, 0, 1.0f));
			FVector  SpreadDirection = SpreadStrength.RotateAngleAxis(-Spread.RecoilOffsetDirections[i], FVector(1.0f, 0, 0));
			mLocalRecoilDirections.Add(SpreadDirection.GetSafeNormal());

			FVector2D FPS_Recoil = FVector2D(SpreadDirection.Y, SpreadDirection.Z).GetSafeNormal();
			mFPS_RecoilDeltaRotations.Add(FPS_Recoil);
		}
	}

	// calculate Salveinterval
	mSalveInterval = mGunProperties.FireCycleInterval / FMath::FloorToInt(mGunProperties.NumSalvesInCycle) * mGunProperties.SalveDistributionInCycle;

	// precalculate Temperature Increase per shot
	if (mGunProperties.MaxContinuousFire > 0) {
		mTempIncreasePercentagePerShot = 1.0f / mGunProperties.MaxContinuousFire;
	} else
	{
		mTempIncreasePercentagePerShot = 0.0f;
	}

	mTracerIndex = 0;
	mProjectileIndex = 0;

	bGunChanged = false;
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "GunComponent: Initialized Gun.");
#endif
}

void UGunFireComponent::StartGunFire()
{
	if (mCurrentStatus == EGunStatus::Idle)
	{
		mCurrentStatus == EGunStatus::Firing;
		switch (mGunProperties.ActionType)
		{
		case EGunActionType::Automatic:
			{
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetWorldTimerManager().SetTimer(mGunFireTimer, this, &UGunFireComponent::GunFireCycle, mGunProperties.FireCycleInterval, true);
					GunFireCycle();
				}
			}
			break;
		case EGunActionType::Triggered:
			{
				GunFireCycle();
			}
			break;
		default: break;
		}
	}
}

void UGunFireComponent::GunFireCycle()
{
	UWorld* World = GetWorld();
	if (World)
	{
		mLastCycleStartTime = World->TimeSeconds;
	}
	AActor* Owner = GetOwner();
	if (Owner)
	{
		mSalveIndex = 0;
		Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, mSalveInterval, true);
		Salve();
	}
}

void UGunFireComponent::Salve()
{
	switch (mCurrentStatus)
	{
	case EGunStatus::Idle:
		{
			mCurrentStatus = EGunStatus::Firing;
			PrepareReloading(true);
		}
		break;
	case EGunStatus::Deactivated: break;
	case EGunStatus::Firing:
		{
			FireSalve();
		}
		break;
	case EGunStatus::Empty: break;
	case EGunStatus::Reloading: break;
	case EGunStatus::Overheated: break;
	case EGunStatus::FinishingCycle:
		{
			FireSalve();
		}
		break;
	case EGunStatus::Error: break;
	default: break;
	}
}

void UGunFireComponent::FireSalve()
{
	if (CheckMagazine())
	{
		HandleRecoil();
		HandleSpread();

		// fire all "Pellets" in case of a shotgun or multiple barrels
		for (int32 i = 0; i < mGunProperties.NumProjectilesInSalve; ++i)
		{
			FireProjectile();
		}

		// increment Projectile index
		if (mGunProperties.ProjectileProperties.Num() > 0) {
			mProjectileIndex = (mProjectileIndex + 1) % mGunProperties.ProjectileProperties.Num();
		}else
		{
			mProjectileIndex = 0;
		}

		// increment Tracer index
		if (mGunProperties.TracerOrder.Num() > 0) {
			mTracerIndex = (mTracerIndex + 1) % mGunProperties.TracerOrder.Num();
		}
		else
		{
			mTracerIndex = 0;
		}

		mRecoilResetSpeed = mRecoilSum.Size();
		UWorld* world = GetWorld();
		if (world)
		{
			mLastTimeFired = world->TimeSeconds;
		}

		DecreaseAmmoInMagazine(1);
		IncreaseTemperature();

		++mSalveIndex;
		// Check salve index
		if (mSalveIndex >= mGunProperties.NumSalvesInCycle)
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				// stop the Salve-Timer by setting the rate to zero
				Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, 0.0f, false);
			}
			CheckMagazine();
		}
	}
}

void UGunFireComponent::HandleRecoil()
{
	TArray<float>& RecoilDirs = mGunProperties.SpreadProperties.RecoilOffsetDirections;
	if (mGunProperties.SpreadProperties.bRecoilIndexFromTemperature)
	{
		// Get Recoil Index from Temp
		if (RecoilDirs.Num() > 0)
		{
			mRecoilIndex = FMath::RoundToInt(mGunOverheatingLevel * (RecoilDirs.Num() - 1));
		}
		else
		{
			mRecoilIndex = 0;
		}
	}
	else
	{
		if (mLocalRecoilDirections.Num() > 0)
		{
			mRecoilIndex = FMath::Min(mRecoilIndex + 1, mLocalRecoilDirections.Num() - 1);
		}
		else
		{
			mRecoilIndex = 0;
		}
	}
	// add Recoil to pending
	if (mFPS_RecoilDeltaRotations.IsValidIndex(mRecoilIndex))
	{
		mPendingRecoilSum += mFPS_RecoilDeltaRotations[mRecoilIndex];
	}
}

void UGunFireComponent::HandleSpread()
{
	mSpread = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f),
	                                            FVector2D(mGunProperties.SpreadProperties.InitialSpread,
	                                                      mGunProperties.SpreadProperties.MaxSpread),
	                                            mGunOverheatingLevel);
}

void UGunFireComponent::FireProjectile()
{
	bool bTracer = true;
	FProjectileProperties Projectile;
	if (mGunProperties.ProjectileProperties.IsValidIndex(mProjectileIndex))
	{
		Projectile = mGunProperties.ProjectileProperties[mProjectileIndex];
	}
	if (mGunProperties.TracerOrder.IsValidIndex(mTracerIndex))
	{
		bTracer = mGunProperties.TracerOrder[mTracerIndex];
	}
	// TODO: recoil/Spread calcs
	// TODO: change this to an Interface Call; update the Gun_Interface
	SpawnProjectile(bTracer, Projectile);
}

void UGunFireComponent::DecreaseAmmoInMagazine(const int32 amount)
{
	mGunProperties.CurrentMagazineLoad = FMath::Max(mGunProperties.CurrentMagazineLoad - amount, 0);
}

void UGunFireComponent::IncreaseTemperature()
{
	mGunOverheatingLevel += FMath::Min(1.0f, mTempIncreasePercentagePerShot);
	CheckOverheated();
}

bool UGunFireComponent::CanStillFire() const
{
	return mGunProperties.CurrentMagazineLoad > 0;
}

bool UGunFireComponent::CheckMagazine()
{
	if (CanStillFire())
	{
		return true;
	}
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// stop the Salve-Timer by setting the rate to zero
		Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, 0.0f, false);
	}
	mbReload = mGunProperties.bAutoReload;
	PrepareReloading(true);
	return false;
}

void UGunFireComponent::CheckOverheated()
{
	if (mGunOverheatingLevel == 1.0f)
	{
		if (mCurrentStatus == EGunStatus::Firing)
		{
			if (mGunProperties.bOverheatingDeactivatesGun)
			{
				mCurrentStatus = EGunStatus::Deactivated;
			}
			else
			{
				mCurrentStatus = EGunStatus::Overheated;
			}
		}
	}
	else
	{
		if (mCurrentStatus == EGunStatus::Deactivated)
		{
			if (mGunProperties.bOverheatingDeactivatesGun && mGunOverheatingLevel == 0.0f)
			{
				mCurrentStatus = EGunStatus::Overheated;
			}
		}
		else if (mCurrentStatus == EGunStatus::Overheated)
		{
			mCurrentStatus = EGunStatus::Idle;
		}
	}
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
