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
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bReplicates = true;
	bAutoActivate = true;

	mbGunChanged = true;
	mCurrentStatus = EGunStatus::Idle;
	mbFiringRequested = false;
	mbUseCycleCooldown = true;
	mRecoilResetFactor = 0;
	mRecoilIndex = 0;
	mRecoilResetSpeed = 0;
	mRelativeSpreadDelta = 0;
	mSpread = 0;
	mSalveInterval = 0;
	mLastCycleStartTime = 0;
	mLastTimeFired = 0;
	mSalveIndex = 0;
	mbReload = false;
	mTempIncreasePercentagePerShot = 0;
	mGunOverheatingLevel = 0;
	mTracerIndex = 0;
	mProjectileIndex = 0;
}


// Called when the game starts
void UGunFireComponent::BeginPlay()
{
	Super::BeginPlay();
	LOG("GunFireComp: BeginPlay");
}


// Called every frame
void UGunFireComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (mGunOverheatingLevel == 0.0f && mPendingRecoilSum.Equals(FVector2D::ZeroVector))
	{
		LOG("GunfireComponent: Stopped Ticking")
			SetComponentTickEnabled(false);
	}
	else
	{
		LOGA("GunfireComponent: Ticking: Overheating-Level = %f", mGunOverheatingLevel)
			LOGA2("GunfireComponent: %f, %f", mPendingRecoilSum.X, mPendingRecoilSum.Y)
	}

	UpdateOwner();

	if (mGunProperties.bSpreadAndRecoilProjectileDynamics)
	{
		AddSmoothRecoil(DeltaTime);
		ReduceRecoil(DeltaTime);
	}
	Cooldown(DeltaTime);



}

void UGunFireComponent::CallStartFiring(const FRandomStream& Stream)
{
	LOG("GunFireComp: CallStartFiring");
	mRandomStream = Stream;
	if (mbGunChanged)
	{
		Initialize();
	}
	mbFiringRequested = mbUseCycleCooldown = true;

	switch (mCurrentStatus)
	{
	case EGunStatus::Idle:	
		StartGunFire();
	break;
	case EGunStatus::Deactivated: break;
	case EGunStatus::Firing: break;
	case EGunStatus::Empty: break;
	case EGunStatus::Reloading:	
		CancelReload();	
	break;
	case EGunStatus::Overheated: break;
	case EGunStatus::FinishingCycle: break;
	case EGunStatus::Error: break;
	default: break;
	}
}

void UGunFireComponent::CallUpdate(const FGunProperties& newProperties)
{
	LOG("GunFireComp: CallUpdate");

	mGunProperties = newProperties;
	mbGunChanged = true;
	PrepareReloading(false);
}

void UGunFireComponent::CallStopFiring()
{
	LOG("GunFireComp: CallStopFiring");

	mbFiringRequested = false;
	UWorld* World = GetWorld();
	if (World)
	{
		mTimeStopRequested = World->TimeSeconds;
	}
	StopFiring();
}

void UGunFireComponent::CallRequestReload()
{
	LOG("GunFireComp: CallRequestReload");

	mbReloadRequested = true;
	switch (mCurrentStatus)
	{
	case EGunStatus::Idle:
	{
		if (CanReload())
		{
			mCurrentStatus = EGunStatus::FinishingCycle;
			Reloading();
		}
		else
		{
			return;
		}
	}
	break;
	case EGunStatus::Deactivated: break;
	case EGunStatus::Firing:
	{
		if (CanReload())
		{
			mCurrentStatus = EGunStatus::FinishingCycle;
			PrepareReloading(false);
		}
		else
		{
			return;
		}
	}
	break;
	case EGunStatus::Empty:
	{
		if (CanReload())
		{
			mCurrentStatus = EGunStatus::FinishingCycle;
			Reloading();
		}
		else
		{
			return;
		}
	}
	break;
	case EGunStatus::Reloading: break;
	case EGunStatus::Overheated: break;
	case EGunStatus::FinishingCycle:
	{
		if (CanReload())
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				// stop the Salve-Timer by setting the rate to zero
				Owner->GetWorldTimerManager().SetTimer(mCycleCooldownTimer, this, &UGunFireComponent::GunCooled, 0.0f);
			}
			Reloading();
		}
		else
		{
			return;
		}
	}
	break;
	case EGunStatus::Error: break;
	default: break;
	}
}

void UGunFireComponent::CallCancelReloadRequest()
{
	LOG("GunFireComp: CallCancelReloadRequest");

	mbReloadRequested = false;
}

void UGunFireComponent::CallAddAmmunition(const int32 Amount)
{
	LOG("GunFireComp: CallAddAmmunition");

	if (Amount > 0)
	{
		mGunProperties.TotalAmmunitionCount += Amount;
		if (mCurrentStatus == EGunStatus::Empty)
		{
			mbReload = true;
			mCurrentStatus = EGunStatus::FinishingCycle;
			Reloading();
		}
	}
}

APlayerController* UGunFireComponent::GetOwningPlayerController() const
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		AController* Controller = Owner->GetInstigatorController();
		if (Controller)
		{
			APlayerController* PlayerCtrl = Cast<APlayerController>(Controller);
			return PlayerCtrl;
		}
	}
	return nullptr;
}

FVector UGunFireComponent::VRandConeFromStream(FVector const& Dir, float ConeHalfAngleRad, const FRandomStream& Stream)
{
	if (ConeHalfAngleRad > 0.f)
	{
		float const RandU = Stream.FRand(); //FMath::FRand();
		float const RandV = Stream.FRand(); //FMath::FRand();

		// Get spherical coords that have an even distribution over the unit sphere
		// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
		float Theta = 2.f * PI * RandU;
		float Phi = FMath::Acos((2.f * RandV) - 1.f);

		// restrict phi to [0, ConeHalfAngleRad]
		// this gives an even distribution of points on the surface of the cone
		// centered at the origin, pointing upward (z), with the desired angle
		Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

		// get axes we need to rotate around
		FMatrix const DirMat = FRotationMatrix(Dir.Rotation());
		// note the axis translation, since we want the variation to be around X
		FVector const DirZ = DirMat.GetScaledAxis(EAxis::X);
		FVector const DirY = DirMat.GetScaledAxis(EAxis::Y);

		FVector Result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
		Result = Result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

		// ensure it's a unit vector (might not have been passed in that way)
		Result = Result.GetSafeNormal();

		return Result;
	}
	else
	{
		return Dir.GetSafeNormal();
	}
}

void UGunFireComponent::Initialize()
{
	LOG("GunFireComp: Initialize");

	const FSpreadProperties& Spread = mGunProperties.SpreadProperties;

	// init Recoil ResetFactor
	const float& RecoilReturnTime = Spread.RecoilReturnTime;
	mRecoilResetFactor = 1.0f / FMath::Max(RecoilReturnTime, 0.001f);

	// precalculate Recoil
	mLocalRecoilDirections.Empty();
	mFPS_RecoilDeltaRotations.Empty();

	for (int i = 0; i < Spread.RecoilOffsetDirections.Num(); ++i)
	{
		if (Spread.InitialRecoil == 0.0f)
		{
			mLocalRecoilDirections.Add(FVector(1.0f, 0.0f, 0.0f));
			mFPS_RecoilDeltaRotations.Add(FVector2D::ZeroVector);
		}
		else
		{
			FVector SpreadStrength = FVector(1.0f, 0, 0).RotateAngleAxis(Spread.InitialRecoil, FVector(0, 0, 1.0f));
			FVector SpreadDirection = SpreadStrength.RotateAngleAxis(-Spread.RecoilOffsetDirections[i], FVector(1.0f, 0, 0));
			mLocalRecoilDirections.Add(SpreadDirection.GetSafeNormal());

			FVector2D FPS_Recoil = FVector2D(SpreadDirection.Y, SpreadDirection.Z).GetSafeNormal();
			mFPS_RecoilDeltaRotations.Add(FPS_Recoil);
		}
	}

	// calculate Salveinterval
	mSalveInterval = mGunProperties.FireCycleInterval / FMath::FloorToInt(mGunProperties.NumSalvesInCycle) * mGunProperties.SalveDistributionInCycle;

	// precalculate Temperature Increase per shot
	if (mGunProperties.MaxContinuousFire > 0)
	{
		mTempIncreasePercentagePerShot = 1.0f / mGunProperties.MaxContinuousFire;
	}
	else
	{
		mTempIncreasePercentagePerShot = 0.0f;
	}

	mTracerIndex = 0;
	mProjectileIndex = 0;

	mbGunChanged = false;
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "GunComponent: Initialized Gun.");
#endif
}

void UGunFireComponent::StartGunFire()
{
	LOG("GunFireComp: StartGunFire");

	if (mCurrentStatus == EGunStatus::Idle)
	{
		mCurrentStatus = EGunStatus::Firing;
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
	LOG("GunFireComp: GunFireCycle");

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
	LOG("GunFireComp: Salve");

	switch (mCurrentStatus)
	{
	case EGunStatus::Idle:
	{
		mCurrentStatus = EGunStatus::Firing;
		PrepareReloading(true);
	}
	break;
	case EGunStatus::Deactivated:
		ClearGunFireAndSalveTimer();
		break;
	case EGunStatus::Firing:
		FireSalve();
	break;
	case EGunStatus::Empty:
		ClearGunFireAndSalveTimer(); 
		break;
	case EGunStatus::Reloading:
		ClearGunFireAndSalveTimer(); 
		break;
	case EGunStatus::Overheated: 
		ClearGunFireAndSalveTimer(); 
		break;
	case EGunStatus::FinishingCycle:
		FireSalve();
	break;
	case EGunStatus::Error:
		ClearGunFireAndSalveTimer(); 
		break;
	default: break;
	}
}

void UGunFireComponent::ClearGunFireAndSalveTimer()
{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, 0.0f, true);
			Owner->GetWorldTimerManager().SetTimer(mGunFireTimer, this, &UGunFireComponent::GunFireCycle, 0.0f, true);
		}
}

void UGunFireComponent::FireSalve()
{
	LOG("GunFireComp: FireSalve");

	if (CheckMagazine())
	{
		if (mGunProperties.bSpreadAndRecoilProjectileDynamics)
		{
			HandleRecoil();
			HandleSpread();
		}

		// fire all "Pellets" in case of a shotgun or multiple barrels
		for (int32 i = 0; i < mGunProperties.NumProjectilesInSalve; ++i)
		{
			FireProjectile();
		}

		SetComponentTickEnabled(true);

		// increment Projectile index
		if (mGunProperties.ProjectileProperties.Num() > 0)
		{
			mProjectileIndex = (mProjectileIndex + 1) % mGunProperties.ProjectileProperties.Num();
		}
		else
		{
			mProjectileIndex = 0;
		}

		// increment Tracer index
		if (mGunProperties.TracerOrder.Num() > 0)
		{
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
				Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, 0.0f, true);
			}
			CheckMagazine();
		}
	}
}

void UGunFireComponent::HandleRecoil()
{
	LOG("GunFireComp: HandleRecoil");

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
	LOG("GunFireComp: HandleSpread");

	mSpread = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f),
		FVector2D(mGunProperties.SpreadProperties.InitialSpread,
			mGunProperties.SpreadProperties.MaxSpread),
		mGunOverheatingLevel);
}

void UGunFireComponent::FireProjectile()
{
	LOG("GunFireComp: FireProjectile");

	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<UGun_Interface>())
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

		// Spread	
		const FVector SpreadDirection = VRandConeFromStream(FVector(1.0f, 0, 0), FMath::DegreesToRadians(mSpread), mRandomStream);
		IGun_Interface::Execute_FireProjectile(Owner, SpreadDirection, Projectile, bTracer);
	}
}

void UGunFireComponent::DecreaseAmmoInMagazine(const int32 amount)
{
	LOG("GunFireComp: DecreaseAmmoInMagazine");

	mGunProperties.CurrentMagazineLoad = FMath::Max(mGunProperties.CurrentMagazineLoad - amount, 0);
}

void UGunFireComponent::IncreaseTemperature()
{
	LOG("GunFireComp: IncreaseTemperature");

	mGunOverheatingLevel = mGunOverheatingLevel + mTempIncreasePercentagePerShot;
	CheckOverheated();
}

bool UGunFireComponent::CanStillFire() const
{
	LOG("GunFireComp: CanStillFire");

	return mGunProperties.CurrentMagazineLoad > 0;
}

bool UGunFireComponent::CheckMagazine()
{
	LOG("GunFireComp: CheckMagazine");

	if (CanStillFire())
	{
		return true;
	}
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// stop the Salve-Timer by setting the rate to zero
		Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, 0.0f, true);
	}
	mbReload = mGunProperties.bAutoReload;
	PrepareReloading(true);
	return false;
}

void UGunFireComponent::CheckOverheated()
{
	// return if  Spread/Recoil is not in use
	if (!mGunProperties.bSpreadAndRecoilProjectileDynamics)
	{
		mGunOverheatingLevel = 0.0f;
		return;
	}

	// Overheated
	if (mGunOverheatingLevel >= 1.0f)
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
		return;
	}

	// not yet overheated or cooling down
		if (mCurrentStatus == EGunStatus::Deactivated)
		{
			// check if gun has completely cooled down
			if (mGunProperties.bOverheatingDeactivatesGun && mGunOverheatingLevel == 0.0f)
			{
				mCurrentStatus = EGunStatus::Overheated;
			}
			return;
		}
		// 
		if (mCurrentStatus == EGunStatus::Overheated && mGunOverheatingLevel < 1.0f)
		{			
			PrepareReloading(true);
		}	
}

void UGunFireComponent::StopFiring()
{
	LOG("GunFireComp: StopFiring");

	if (mCurrentStatus == EGunStatus::Firing)
	{
		CalcCycleRemainingTime();
		mCurrentStatus = EGunStatus::FinishingCycle;
		AActor* Owner = GetOwner();
		if (Owner)
		{
			if (mCycleRemainingTime > 0.0f)
			{
				Owner->GetWorldTimerManager().SetTimer(mCycleCooldownTimer, this, &UGunFireComponent::GunCooled, mCycleRemainingTime, false);
			}
			else
			{
				GunCooled();
			}
		}
	}
}

void UGunFireComponent::CalcCycleRemainingTime()
{
	LOG("GunFireComp: CalcCycleRemainingTime");

	if (mbUseCycleCooldown)
	{
		if (mGunProperties.ActionType == EGunActionType::Automatic)
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				Owner->GetWorldTimerManager().PauseTimer(mGunFireTimer);
				mCycleRemainingTime = Owner->GetWorldTimerManager().GetTimerRemaining(mGunFireTimer);
				Owner->GetWorldTimerManager().SetTimer(mGunFireTimer, this, &UGunFireComponent::GunFireCycle, 0.0f, true);
				Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, 0.0f, true);
			}
		}
		else
		{
			mCycleRemainingTime = GetSemiAutoCooldown();
		}
	}
	else
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			Owner->GetWorldTimerManager().SetTimer(mGunFireTimer, this, &UGunFireComponent::GunFireCycle, 0.0f, true);
			Owner->GetWorldTimerManager().SetTimer(mSalveTimer, this, &UGunFireComponent::Salve, 0.0f, true);
		}
		mCycleRemainingTime = 0.0f;
	}
}

float UGunFireComponent::GetSemiAutoCooldown()
{
	LOG("GunFireComp: GetSemiAutoCooldown");

	UWorld* World = GetWorld();
	if (World)
	{
		float CoolingStartTime = mGunProperties.bReleaseForCooldown ? mTimeStopRequested : mLastCycleStartTime;
		float RemainingCooldownTime = mGunProperties.FireCycleInterval - (World->TimeSeconds - CoolingStartTime);
		return FMath::Max(0.0f, RemainingCooldownTime);
	}
	return 0.0f;
}

void UGunFireComponent::GunCooled()
{
	LOG("GunFireComp: GunCooled");

	//AActor* Owner = GetOwner();
	//if (Owner)
	//{
	//	// Clear CycleCooldownTimer
	//	Owner->GetWorldTimerManager().SetTimer(mCycleCooldownTimer, this, &UGunFireComponent::GunCooled, 0.0f);
	//}
	if (mCurrentStatus == EGunStatus::FinishingCycle)
	{
		CheckRequests();
	}
}

void UGunFireComponent::CheckRequests()
{
	LOG("GunFireComp: CheckRequests");

	if (mbGunChanged) Initialize();
	if (mbDeactivationRequested)
	{
		mCurrentStatus = EGunStatus::Deactivated;
		return;
	}
	if ((mbReload || mbReloadRequested) && CanReload())
	{
		mCurrentStatus = EGunStatus::FinishingCycle;
		Reloading();
		return;
	}
	if (CanStillFire())
	{
		mCurrentStatus = EGunStatus::Idle;
		if (mbFiringRequested && mGunProperties.bAutoContinueFire && mGunProperties.ActionType == EGunActionType::Automatic)
		{
			StartGunFire();
		}
		return;
	}
	mCurrentStatus = EGunStatus::Empty;
}

bool UGunFireComponent::CanReload()
{
	LOG("GunFireComp: CanReload");

	const bool bHasRefill = mGunProperties.TotalAmmunitionCount > 0;
	const bool MagNotFull = mGunProperties.CurrentMagazineLoad < mGunProperties.MagazineSize;
	const bool AdditionalRound = (mGunProperties.MagazineSize == mGunProperties.CurrentMagazineLoad) && mGunProperties.bAdditionalChamberRound;
	return bHasRefill && (MagNotFull ^ AdditionalRound);
}

void UGunFireComponent::PrepareReloading(const bool bUseCycleCooldown)
{
	LOG("GunFireComp: PrepareReloading");

	mbUseCycleCooldown = bUseCycleCooldown;
	mCurrentStatus = EGunStatus::Firing;
	StopFiring();
}

void UGunFireComponent::Reloading()
{
	LOG("GunFireComp: Reloading");

	if (mCurrentStatus == EGunStatus::FinishingCycle)
	{
		mCurrentStatus = EGunStatus::Reloading;
		mNumAddedProjectiles = 0;
		if (mGunProperties.MagazineSize == mGunProperties.CurrentMagazineLoad)
		{
			if (mGunProperties.bAdditionalChamberRound)
			{
				AddChamberRound();
			}
			else
			{
				ReloadCancelled();
			}
		}
		else
		{
			switch (mGunProperties.ReloadType)
			{
			case EReloadType::Magazine:
			{
				mReloadTime = mGunProperties.ReloadTimeWholeMagazine;
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, mReloadTime, false);
				}
			}
			break;
			case EReloadType::Single:
			{
				mReloadTime = mGunProperties.ReloadTimeSingleProjectile;
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, mReloadTime, true);
				}
			}
			break;
			case EReloadType::BoltAction:
			{
				mReloadTime = mGunProperties.ReloadTimeSingleProjectile;
				AActor* Owner = GetOwner();
				if (Owner)
				{
					Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, mReloadTime, false);
				}
			}
			break;
			default: break;
			}
		}
	}
}

void UGunFireComponent::AddChamberRound()
{
	LOG("GunFireComp: AddChamberRound");

	mbIsChamberRound = true;
	mReloadTime = mGunProperties.ReloadTimeSingleProjectile;
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, mReloadTime, false);
	}
}

void UGunFireComponent::ReloadingFinished()
{
	LOG("GunFireComp: ReloadingFinished");

	if (mbIsChamberRound)
	{
		mbIsChamberRound = false;
		AActor* Owner = GetOwner();
		if (Owner)
		{
			// Clear ReloadTimer
			Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, 0.0f);
		}
		FillMagazine(1);
	}
	else
	{
		switch (mGunProperties.ReloadType)
		{
		case EReloadType::Magazine:
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				// Clear ReloadTimer
				Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, 0.0f);
			}
			FillMagazine(
				FMath::Min(
					mGunProperties.MagazineSize - mGunProperties.CurrentMagazineLoad,
					mGunProperties.MaxMagazineReloadAmount
				)
			);
		}
		break;
		case EReloadType::Single:
		{
			FillMagazine(1);
			if (mbFinishedReloading || mbFiringRequested)
			{
				AActor* Owner = GetOwner();
				if (Owner)
				{
					// Clear ReloadTimer
					Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, 0.0f);
				}
			}
			else
			{
				return;
			}
		}
		break;
		case EReloadType::BoltAction:
		{
			AActor* Owner = GetOwner();
			if (Owner)
			{
				// Clear ReloadTimer
				Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, 0.0f);
			}
			FillMagazine(1);
		}
		break;
		default: break;
		}
	}
	mbReload = false;
	CheckRequests();
}

void UGunFireComponent::FillMagazine(const int32 toAdd)
{
	LOG("GunFireComp: FillMagazine");

	int32 Refill = toAdd;
	int32 newTotal = mGunProperties.TotalAmmunitionCount - Refill;
	if (newTotal < 0)
	{
		Refill += newTotal;
	}
	mGunProperties.TotalAmmunitionCount = FMath::Max(0, newTotal);
	mGunProperties.CurrentMagazineLoad += Refill;
	mNumAddedProjectiles += Refill;

	mbFinishedReloading =
		Refill == 0
		|| mNumAddedProjectiles >= mGunProperties.MaxMagazineReloadAmount
		|| mGunProperties.MagazineSize == mGunProperties.CurrentMagazineLoad;
}

void UGunFireComponent::CancelReload()
{
	LOG("GunFireComp: CancelReload");

	if (mCurrentStatus == EGunStatus::Reloading && CanStillFire())
	{
		ReloadCancelled();
	}
}

void UGunFireComponent::ReloadCancelled()
{
	LOG("GunFireComp: ReloadCancelled");

	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->GetWorldTimerManager().SetTimer(mReloadTimer, this, &UGunFireComponent::ReloadingFinished, 0.0f);
	}
	mbReload = false;
	CheckRequests();
}

void UGunFireComponent::AddSmoothRecoil(const float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<UGun_Interface>())
	{
		APawn* Pawn = IGun_Interface::Execute_GetOwningPawn(Owner);
		if (Pawn) {
			FVector2D RecoilToApply = mPendingRecoilSum * FMath::Min(1.0f, DeltaTime * mGunProperties.SpreadProperties.RecoilVelocity);
			mPendingRecoilSum -= RecoilToApply;
			Pawn->AddControllerYawInput(RecoilToApply.X);
			Pawn->AddControllerPitchInput(RecoilToApply.Y);
			mRecoilSum += RecoilToApply;
		}
		else
		{
			mPendingRecoilSum = FVector2D::ZeroVector;
		}
	}
}

void UGunFireComponent::ReduceRecoil(const float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (Owner && Owner->Implements<UGun_Interface>())
	{
		APawn* Pawn = IGun_Interface::Execute_GetOwningPawn(Owner);
		if (Pawn) {
			FVector2D ConstResult = FMath::Vector2DInterpConstantTo(
				mRecoilSum,
				FVector2D::ZeroVector,
				DeltaTime,
				mRecoilResetSpeed * mRecoilResetFactor);

			FVector2D InterpResult = FMath::Vector2DInterpTo(
				mRecoilSum,
				FVector2D::ZeroVector,
				DeltaTime,
				mGunProperties.SpreadProperties.RecoilDecreaseSpeed);

			FVector2D& ResultRecoilSum = InterpResult;
			if (ConstResult.SizeSquared() < InterpResult.SizeSquared())
			{
				ResultRecoilSum = ConstResult;
			}

			FVector2D Delta = ResultRecoilSum - mRecoilSum;
			Pawn->AddControllerYawInput(Delta.X);
			Pawn->AddControllerPitchInput(Delta.Y);
			mRecoilSum = ResultRecoilSum;
		}
	}
}

void UGunFireComponent::Cooldown(const float DeltaTime)
{
	if (mCurrentStatus == EGunStatus::Error) return;
	if ((mCurrentStatus == EGunStatus::Firing || mCurrentStatus == EGunStatus::FinishingCycle) && !mGunProperties.bCanCooldownWhileFiring)
	{
		return;
	}
	if (mGunProperties.CooldownType == ECooldownType::ConstantSpeed)
	{
		mGunOverheatingLevel = FMath::FInterpConstantTo(
			mGunOverheatingLevel,
			0.0f,
			DeltaTime,
			1.0f / FMath::Max(mGunProperties.CoolDownTime, 0.001f));
	}
	else
	{
		mGunOverheatingLevel = FMath::FInterpTo(
			mGunOverheatingLevel,
			0.0f,
			DeltaTime,
			mGunProperties.RelativeCoolDownSpeed);
	}
	CheckOverheated();
}

void UGunFireComponent::GetCurrentGunProperties(FGunProperties& Properties) const
{
	Properties = mGunProperties;
}

float UGunFireComponent::GetRemainingReloadTime() const
{
	float remainingTime = 0.0f;
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetWorldTimerManager().IsTimerActive(mReloadTimer))
	{
		remainingTime = Owner->GetWorldTimerManager().GetTimerRemaining(mReloadTimer);
	}
	return remainingTime;
}

float UGunFireComponent::GetReloadTime() const
{
	float reloadTime = 0.0f;
	AActor* Owner = GetOwner();
	if (Owner && Owner->GetWorldTimerManager().IsTimerActive(mReloadTimer))
	{
		reloadTime = Owner->GetWorldTimerManager().GetTimerRate(mReloadTimer);
	}
	return reloadTime;
}

float UGunFireComponent::GetOverheatingLevel() const
{
	return FMath::Min(1.0f, mGunOverheatingLevel);
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
