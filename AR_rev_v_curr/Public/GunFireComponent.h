// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Projectile.h"
#include "Gun_Interface.h"
#include "GunFireComponent.generated.h"

// Enum to define a Guns Type: Automatic/Triggered
UENUM(BlueprintType)
enum class EGunActionType : uint8
{
	// Fires continuously while the Gun is triggered
	Automatic 	        UMETA(DisplayName = "Automatic-Type"),

	// Fires once every time the Gun is triggered
	Triggered			UMETA(DisplayName = "Triggered-Type")
};

UENUM(BlueprintType)
enum class EGunStatus : uint8
{
	// ready to Fire
	Idle  UMETA(DisplayName = "Idle"),

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
struct FSpreadProperties {
	GENERATED_USTRUCT_BODY()
		// initial Random Spread in Degree
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float InitialSpread;

	// multiplier for the next random spread
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float SpreadIncreaseFactor;

	// max spread used for random generation of spread in Degree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float MaxSpread;

	// initial fixed amount of change in direction in Degree
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float InitialRecoil;

	// multiplier to decrease next change in direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float RecoilDecreaseFactor;

	// values that define the direction of the recoil in range of +180 to -180 Degree
	// examples:
	// 0 -> recoil right
	// +180 or -180 -> recoil left
	// +90 -> recoil up
	// -90 -> recoil down
	//(all in local Space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		TArray<float> RecoilOffsetDirections;

	// Time it takes to go back to the initial Recoil, changing Firerates or Cooldowntime can change the Overall recoil
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings|Spread")
		float RecoveryTime;
};

UENUM(BlueprintType)
enum class EReloadType : uint8
{
	// reload Multiple Rounds at once, if aborted no Rounds are loaded
	Magazine  UMETA(DisplayName = "Magazine"),
	// reload one round after another, if aborted all completely loaded rounds stay loaded
	Single  UMETA(DisplayName = "Single"),
	// reload one round and reloading is finished, if aborted no Rounds are loaded
	BoltAction  UMETA(DisplayName = "BoltAction")
};

// Struct that defines a Guns Characteristics
USTRUCT(BlueprintType)
struct FGunProperties {
	GENERATED_USTRUCT_BODY()

		// Number of Projectiles available for firing
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 TotalAmmunitionCount = 600;

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
	EReloadType ReloadType;

	// after reloading one additional round can be loaded; Max Projectiles to Fire = MagazineSize + 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
	bool bAdditionalChamberRound = true;

	// Time until a new full Magazine is available
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float ReloadTimeWholeMagazine;

	// Time to load a single Projectile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float ReloadTimeSingleProjectile;

	// Time for one Firingcyle in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float FireCycleInterval = 1.0f;

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
		TArray<bool> TracerOrder;



	// Number of Projectiles that can be fired continuously to get from cold to overheated Status
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		int32 MaxContinuousFire;

	// Total Time it takes to cool an Overheated Gun down so that it can Fire again
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		float CoolDownTime;

	// automatic or triggeraction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		EGunActionType ActionType;

	// Array of structs that holds all the different Projectile-Types this gun can Fire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		TArray<FProjectileProperties> ProjectileProperties;

	// recoil of the gun generates impulses to the guns owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bPhysicalRecoil;

	// activate spread and recoil
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gun|Settings")
		bool bSpreadAndRecoilProjectileDynamics;

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

	// struct that holds all important properties that defines the Guns Characteristics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Gun|Settings")
		FGunProperties GunProperties;

	// Current Operational Status of the Gun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Gun|Settings")
		EGunStatus CurrentStatus = EGunStatus::Idle;


	void Initialize(const FGunProperties& GunProperties);

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void StartFiring(class UPrimitiveComponent* GunBarrel,
			const TArray<FName>& GunSockets);

	void StartGunFire();

	void StartFiringCycle();

	void GunFireSalve();

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
		void StopGunFire();

	UFUNCTION(BlueprintCallable, Category = "GunFireComponent")
	void ReloadMagazine();

	void GunCooldownElapsed();
	void ReloadingFinished();;


	UFUNCTION(BlueprintNativeEvent, Category = "GunFireComponent")
		void SpawnProjectile(const FTransform &SocketTransform, const bool bTracer, const FVector &FireBaseVelocity = FVector::ZeroVector, const FVector &TracerStartLocation = FVector::ZeroVector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunFireComponent")
		TArray<FName> GunSockets;

	//Projectile class to spawn
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AProjectile> ProjectileClass;



private:
	class UPrimitiveComponent* GunBarrel;
	bool bGunFireRequested;
	bool bGunReady;
	int32 AmmoAmount;
	float BaseFireInterval;
	// Time between Salves
	float GunSalveIntervall;
	int32 GunCurrentSalve;
	int32 GunNumSalves;
	int32 NumProjectiles;
	int32 CurrGunSocketIndex;
	float WeaponSpreadRadian;
	float GunRecoilForce;
	FTimerHandle GunFireHandle;
	FTimerHandle GunFireCycleCooldown;
	FTimerHandle GunFireOverheatingCooldown;
	FTimerHandle GunReloadCooldown;
	FTimerHandle GunSalveTimerHandle;
	int32 TracerIntervall = 1;
	int32 CurrentTracer;

	// checks the Magazine for Ammo
	inline bool MagHasAmmo() const;

	// returns true if there is no Ammunition left to refill a Magazine
	inline bool IsOutOfAmmo() const;
};
