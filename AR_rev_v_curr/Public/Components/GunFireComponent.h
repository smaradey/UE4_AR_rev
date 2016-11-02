// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Projectile.h"
#define LOG_MSG 0
#include "CustomMacros.h"
#include "GunFireComponent.generated.h"

// Enum to define a Guns Type: Automatic/Triggered
UENUM(BlueprintType)
enum class EGunActionType : uint8
{
	// Fires continuously while the Gun is triggered
	Automatic UMETA(DisplayName = "Automatic-Type"),

	// Fires once every time the Gun is triggered
	Triggered UMETA(DisplayName = "Triggered-Type")
};

UENUM(BlueprintType)
enum class EGunStatus : uint8
{
	// ready to Fire
	Idle UMETA(DisplayName = "Idle"),

	// deactivated, can not Fire and not relaod but cool down
	Deactivated UMETA(DisplayName = "Deactivated"),

	// fires until magazine is empty or overheats
	Firing UMETA(DisplayName = "Firing"),

	// can not reload and not fire but cool down
	Empty UMETA(DisplayName = "Empty"),

	// can not fire but cool down
	Reloading UMETA(DisplayName = "Reloading"),

	// can reload and cool down but not fire
	Overheated UMETA(DisplayName = "Overheated"),

	// can reload and cool down but not fire
	FinishingCycle UMETA(DisplayName = "FinishingCycle"),

	// In case Something went wrong
	Error UMETA(DisplayName = "Error")
};

USTRUCT(BlueprintType)
struct FWeaponStatus
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Status")
		EGunStatus Status = EGunStatus::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Status")
		float Temperature = 0.0f;
};

USTRUCT(BlueprintType)
struct FSpreadProperties
{
	GENERATED_USTRUCT_BODY()
		// initial Random Spread in Degree
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float InitialSpread = 0.02f;

	// max spread used for random generation of spread in Degree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float MaxSpread = 1.0f;

	// initial fixed amount of change of direction in Degree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float InitialRecoil = 1.0f;

	// Speed to decrease next change in direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float RecoilDecreaseSpeed = 2.0f;

	// values that define the direction of the recoil in range of +180 to -180 Degree
	// examples:
	// 0 -> recoil right
	// +180 or -180 -> recoil left
	// +90 -> recoil up
	// -90 -> recoil down
	//(all in local Space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		TArray<float> RecoilOffsetDirections = { 90.0f };

	// if true the current temperature choose the recoil-direction-index;
	// if false every shot increments the index by one
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bRecoilIndexFromTemperature = true;

	// Time it takes to return to the initial aim direction after the gun stops firing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float RecoilReturnTime = 0.1f;
};

UENUM(BlueprintType)
enum class EReloadType : uint8
{
	// reload Multiple Rounds at once, if aborted no Rounds are loaded
	Magazine UMETA(DisplayName = "Magazine"),
	// reload one round after another, if aborted all completely loaded rounds stay loaded
	Single UMETA(DisplayName = "Single"),
	// reload one round and reloading is finished, if aborted no Rounds are loaded
	BoltAction UMETA(DisplayName = "BoltAction")
};

UENUM(BlueprintType)
enum class ECooldownType : uint8
{
	// use constant CooldownSpeed, cool down by a constant amount regardless of Temperature-Level (use FinterpToConstant)
	ConstantSpeed UMETA(DisplayName = "ConstantSpeed"),
	// use relative CooldownSpeed, cool down using the given CooldownSpeed (use FInterp)
	RelativeSpeed UMETA(DisplayName = "RelativeSpeed"),
};

// Struct that defines a Guns Characteristics
USTRUCT(BlueprintType)
struct FGunProperties
{
	GENERATED_USTRUCT_BODY()

		// Number of Projectiles available for firing
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 TotalAmmunitionCount = 300;

	// Number of Projectiles that can be fired before a reload is needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 MagazineSize = 30;

	// Number of Projectiles that can be fired before a reload is needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 CurrentMagazineLoad = 30;

	// Number of Projectiles that can loaded into the magazine at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 MaxMagazineReloadAmount = 30;

	// Reload Type: Magazine, Single or BoltAction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		EReloadType ReloadType = EReloadType::Magazine;

	// reload automatically when Magazine epmty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bAutoReload = false;

	// whether the gun automatically continues firing after reloading when firing is still requested
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bAutoContinueFire = false;

	// after reloading one additional round can be loaded; Max Projectiles to Fire = MagazineSize + 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bAdditionalChamberRound = true;

	// Time until a new full Magazine is available
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float ReloadTimeWholeMagazine = 2.0f;

	// Time to load a single Projectile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float ReloadTimeSingleProjectile = 1.0f;

	// Time for one Firingcyle in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float FireCycleInterval = 0.1f;

	// true: can shot again after a time of FireCycleInterval has passed after a shot
	// false: can shot again after the gun was requested to stop firing and a time of FireCycleInterval has passed
	// only affects semi-automatic Gunfire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bImmediatelyFinishGunCylce = true;

	// Number of Salves in one Firingcycle (at least one)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 NumSalvesInCycle = 1;

	// Number of Projectiles in one Salve (at least one, more and the gun behaves like a shotgun for example)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 NumProjectilesInSalve = 1;

	// value that defines how the salves are distributed in one firingcycle
	// Example 1:
	// NumSalvesInCycle = 2
	// FireCycleInterval = 1.0 seconds
	// SalveDistributionInCycle = 0.5
	// Result when Gun fires:
	// | Shot -> 0.25s -> Shot -> 0.75s | Shot -> 0.25s -> Shot -> 0.75s
	//
	// Example 2:
	// NumSalvesInCycle = 3
	// FireCycleInterval = 1.0 seconds
	// SalveDistributionInCycle = 1.0
	// Result when Gun fires:
	// | Shot -> 0.33s -> Shot -> 0.33s -> Shot -> 0.33s | Shot -> 0.33s -> Shot -> 0.33s -> Shot -> 0.33s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float SalveDistributionInCycle = 1.0f;

	// Array of Boolean that allows creating a custom Tracer order
	// set to true for Tracers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		TArray<bool> TracerOrder = { true };

	// Number of Projectiles that can be fired continuously to get from cold to overheated Status
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 MaxContinuousFire = 30;

	// if true the Gun has to cool down entirely in order to fire/reload again.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bOverheatingDeactivatesGun = true;

	// Total Time it takes to cool an Overheated Gun down so that it can Fire again
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float CoolDownTime = 5.0f;

	// automatic or triggeraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		EGunActionType ActionType = EGunActionType::Automatic;

	// Array of structs that holds all the different Projectile-Types this gun can Fire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		TArray<FProjectileProperties> ProjectileProperties;

	// recoil of the gun generates impulses to the guns owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bPhysicalRecoil = false;

	// activate spread and recoil
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bSpreadAndRecoilProjectileDynamics = true;

	// Properties of the spreads behaviour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		FSpreadProperties SpreadProperties;

};

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

public:
	// Callable by the owner of this component to start firing
	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallStartFiring(const FRandomStream& Stream);

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallUpdate(const FGunProperties& newProperties);

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallStopFiring();

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallRequestReload();

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallCancelReloadRequest();

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void CallAddAmmunition(const int32 Amount);

	// converts a vectors rotation to Worldspace
	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		static FTransform ConvertSpreadToWorld(const AActor* Actor, const FTransform& RelativeSpawnTransform, const FVector& RelativeSpreadDirection)
	{
		// RotationResult = B.Rotation * A.Rotation
		//OutTransform->Rotation = VectorQuaternionMultiply2(QuatB, QuatA);
		if (Actor)
		{
			const FTransform& ActorTransform = Actor->GetTransform();
			FTransform Result = RelativeSpawnTransform;
			Result.ConcatenateRotation(FQuat(RelativeSpreadDirection.Rotation()));
			return Result * ActorTransform;
		}
		return FTransform();
	}

	// return the player controller of the owners instigator or null
	UFUNCTION(BlueprintCallable, Category = "GunFireComponent|Owner")
		APlayerController* GetOwningPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		static FVector VRandConeFromStream(FVector const& Dir, float ConeHalfAngleRad, const FRandomStream& Stream);

	// the time it takes to add the recoil of one shot to the pawn is 1.0/mRecoilVelocity seconds, eg. 1.0/10 = 0.1 [Seconds]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent|Recoil", meta = (ClampMin = "0.01", ClampMax = "1000.0", UIMin = "0.01", UIMax = "100.0"))
		float mRecoilVelocity = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent|Cooldown")
		bool mbCanCooldownWhileFiring = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent|Cooldown")
		ECooldownType mCooldownType = ECooldownType::ConstantSpeed;

	// use this when using ECooldownType::RelativeSpeed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent|Cooldown")
		float mRelativeCoolDownSpeed = 0.25f;


private:
	void Initialize();
	void StartGunFire();
	void GunFireCycle();
	void Salve();
	void FireSalve();
	void HandleRecoil();
	FORCEINLINE void HandleSpread();
	void FireProjectile();
	FORCEINLINE void DecreaseAmmoInMagazine(const int32 amount);
	void IncreaseTemperature();
	FORCEINLINE bool CanStillFire() const;
	bool CheckMagazine();
	void CheckOverheated();
	void StopFiring();
	void CalcCycleRemainingTime();
	float GetSemiAutoCooldown();
	void GunCooled();
	void CheckRequests();
	bool CanReload();
	void PrepareReloading(const bool bUseCycleCooldown);
	void Reloading();
	void AddChamberRound();
	void ReloadingFinished();
	void FillMagazine(const int32 toAdd);
	void CancelReload();
	void ReloadCancelled();
	void AddSmoothRecoil(const float DeltaTime);
	void ReduceRecoil(const float DeltaTime);
	void Cooldown(const float DeltaTime);




public:
	// Current Status of the Gun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Gun|Settings")
		EGunStatus mCurrentStatus;

private:
	// Settings
	FGunProperties mGunProperties;
	FRandomStream mRandomStream;

	// requests
	bool mbGunChanged;
	bool mbFiringRequested;
	bool mbUseCycleCooldown;
	bool mbDeactivationRequested;
	bool mbReloadRequested;

	// Recoil	
	float mRecoilResetFactor;
	TArray<FVector> mLocalRecoilDirections; // Recoil Direction in local Space
	TArray<FVector2D> mFPS_RecoilDeltaRotations; // Recoil that is being applied to the owning controller (x=yaw, y=pitch [deg])
	FVector2D mRecoilSum;
	FVector2D mPendingRecoilSum;
	int32 mRecoilIndex;
	float mRecoilResetSpeed;

	// Spread
	float mRelativeSpreadDelta;
	float mSpread;

	// Gunfire
	float mSalveInterval;
	FTimerHandle mGunFireTimer;
	FTimerHandle mCycleCooldownTimer;
	float mCycleRemainingTime;
	float mLastCycleStartTime;
	float mLastTimeFired;
	float mTimeStopRequested;
	FTimerHandle mSalveTimer;
	int32 mSalveIndex;

	// Reloading
	bool mbReload;
	float mReloadTime;
	FTimerHandle mReloadTimer;
	int32 mNumAddedProjectiles;
	bool mbIsChamberRound;
	bool mbFinishedReloading;

	// Overheating
	float mTempIncreasePercentagePerShot;
	float mGunOverheatingLevel;
	// Projectile
	int32 mTracerIndex;
	int32 mProjectileIndex;


public:
	//Projectile class to spawn
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AProjectile> ProjectileClass;


	void UpdateOwner() const;


};
