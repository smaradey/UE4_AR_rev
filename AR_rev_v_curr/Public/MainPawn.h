// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "AR_rev_v_curr.h"
#include "GameFramework/Pawn.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "UnrealNetwork.h"
#include "Target_Interface.h"
#include "MainPawn.generated.h"


// Enum to select a specific Turn-Algorithm (deprecated)
UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class DebugTurning : uint8
{
	Default 	        UMETA(DisplayName = "Default"),
	SmoothWithFastStop 	UMETA(DisplayName = "SmoothWithFastStop"),
	Smooth	            UMETA(DisplayName = "Smooth")
};


/*
Struct to send Player-Input-Data (Compressed to 64 bit) to server:
Movement-Input: Forward/Side [left,right,forward,backwards]
Cumulative Mouse-Input: Up/Right [-1.0..1.0]
Packet-Number
Gun-Button pressed: [true,false]
Missile-Button pressed: [true,false]
Target-Lock-Button pressed: [true,false]
*/
USTRUCT()
struct FPlayerInputPackage {
	GENERATED_USTRUCT_BODY()
		// Default Constructor
		FPlayerInputPackage() {
		PlayerDataCompressed = 0LLU;
	};

	// Constructor to initialize the Struct
	FPlayerInputPackage(bool Gun, bool Missile, bool Lock, const FVector2D & MovementInput, const FVector2D &MouseInput, const uint32 newPacketNumber) {
		PlayerDataCompressed = 0LLU;
		setGunFire(Gun);
		setMissileFire(Missile);
		setSwitchTarget(Lock);
		SetMovementInput(MovementInput);
		setMouseInput(MouseInput);
		setPacketNumber(newPacketNumber);
	};

	/* TODO */
	UPROPERTY()
		uint64 PlayerDataCompressed;

	// 4 bit 64-4 = 60
	/* TODO */
	void SetMovementInput(const FVector2D & MovementInput) {
		uint64 const rightPressed = 1LLU;
		uint64 const leftPressed = 1LLU << 1;
		uint64 const forwardPressed = 1LLU << 2;
		uint64 const backwardsPressed = 1LLU << 3;
		uint64 const clearMovementInput = ~15LLU;

		PlayerDataCompressed &= clearMovementInput;
		// forward
		if (MovementInput.X > 0.0f) {
			PlayerDataCompressed |= forwardPressed;
		}
		else
			// backwards
			if (MovementInput.X < 0.0f) {
				PlayerDataCompressed |= backwardsPressed;
			}
		// right
		if (MovementInput.Y > 0.0f) {
			PlayerDataCompressed |= rightPressed;
		}
		else
			// left
			if (MovementInput.Y < 0.0f) {
				PlayerDataCompressed |= leftPressed;
			}
	}

	/* TODO */
	FVector2D GetMovementInput() const {
		uint64 const rightPressed = 1LLU;
		uint64 const leftPressed = 1LLU << 1;
		uint64 const forwardPressed = 1LLU << 2;
		uint64 const backwardsPressed = 1LLU << 3;
		FVector2D Input = FVector2D::ZeroVector;
		// forward
		if ((PlayerDataCompressed & forwardPressed) == forwardPressed) {
			Input.X = 1.0f;
		}
		// backwards
		if ((PlayerDataCompressed & backwardsPressed) == backwardsPressed) {
			Input.X = -1.0f;
		}
		// right
		if ((PlayerDataCompressed & rightPressed) == rightPressed) {
			Input.Y = 1.0f;
		}
		// left
		if ((PlayerDataCompressed & leftPressed) == leftPressed) {
			Input.Y = -1.0f;
		}
		return Input;
	}

	// 24 bit 60-24 = 36
	/* TODO */
	void setMouseInput(const FVector2D &MouseInput) {
		uint64 const mouseInputClear = ~(16777215LLU << 4);
		uint64 const mouseYawIsNegative = 1LLU << 15;
		uint64 const mousePitchIsNegative = 1LLU << 27;

		PlayerDataCompressed &= mouseInputClear;

		PlayerDataCompressed |= ((uint64)(2047 * FMath::Abs(MouseInput.X))) << 4;
		if (MouseInput.X < 0.0f) {
			PlayerDataCompressed |= mouseYawIsNegative;
		}

		PlayerDataCompressed |= ((uint64)(2047 * FMath::Abs(MouseInput.Y))) << 16;
		if (MouseInput.Y < 0.0f) {
			PlayerDataCompressed |= mousePitchIsNegative;
		}
	}

	/* TODO */
	float getMouseYaw() const {
		uint64 const mouseYaw = 2047LLU << 4;
		uint64 const mouseYawIsNegative = 1LLU << 15;

		if ((PlayerDataCompressed & mouseYawIsNegative) == mouseYawIsNegative) {

			return 0.0f - (((PlayerDataCompressed & mouseYaw) >> 4) / 2047.0f);

		}
		return ((PlayerDataCompressed & mouseYaw) >> 4) / 2047.0f;
	}

	/* TODO */
	float getMousePitch() const {
		uint64 const mousePitch = 2047LLU << 16;
		uint64 const mousePitchIsNegative = 1LLU << 27;

		if ((PlayerDataCompressed & mousePitchIsNegative) == mousePitchIsNegative) {
			return 0.0f - (((PlayerDataCompressed & mousePitch) >> 16) / 2047.0f);
		}
		return ((PlayerDataCompressed & mousePitch) >> 16) / 2047.0f;
	}

	/* TODO */
	FVector2D getMouseInput() const {
		return FVector2D(getMouseYaw(), getMousePitch());
	}

	/* TODO */
	void setPacketNumber(const uint32 newPacketNumber) {
		uint64 const PacketNoClear = ~(16777215LLU << 28);
		PlayerDataCompressed &= PacketNoClear;
		PlayerDataCompressed |= ((uint64)newPacketNumber) << 28;
	}

	/* TODO */
	void IncrementPacketNumber() {
		setPacketNumber(1 + getPacketNumber());
	}

	// 24 bit 36-24 = 12
	/* TODO */
	uint32 getPacketNumber() const {
		uint64 const PacketNo = 16777215LLU << 28;
		return (uint32)((PlayerDataCompressed & PacketNo) >> 28);
	}

	// 1 bit 64th  11
	/* TODO */
	void setGunFire(const uint32 bGunFire) {
		setBit(bGunFire, 63);
	}

	/* TODO */
	bool getGunFire() const {
		return (PlayerDataCompressed & (1LLU << 63)) == (1LLU << 63);
	}

	// 1 bit 63th 10
	/* TODO */
	void setMissileFire(const uint32 bMissileFire) {
		setBit(bMissileFire, 62);
	}

	/* TODO */
	bool getMissileFire() const {
		return (PlayerDataCompressed & (1LLU << 62)) == (1LLU << 62);
	}

	// help function to reduce redundancy
	void setBit(const uint32 bSet, const int Position) {
		if (bSet) {
			PlayerDataCompressed |= (1LLU << Position);
		}
		else {
			PlayerDataCompressed &= ~(1LLU << Position);
		}
	}

	// 1 bit 62th 9
	/* TODO */
	void setSwitchTarget(const uint32 bSwitchTarget) {
		setBit(bSwitchTarget, 61);
	}

	/* TODO */
	bool getSwitchTarget() const {
		return (PlayerDataCompressed & (1LLU << 61)) == (1LLU << 61);
	}

	// 1 bit 61th 8
	/* TODO */
	void setGunLock(const uint32 bGunLock) {
		setBit(bGunLock, 60);
	}

	/* TODO */
	bool getGunLock() const {
		return (PlayerDataCompressed & (1LLU << 60)) == (1LLU << 60);
	}

	// 1 bit 60th 7
	/* TODO */
	void setMissileLock(const uint32 bMissileLock) {
		setBit(bMissileLock, 59);
	}

	/* TODO */
	bool getMissileLock() const {
		return (PlayerDataCompressed & (1LLU << 59)) == (1LLU << 59);
	}


};

/* Struct that stores the current Pawn-Transform with the corresponding Packet-Number of the Player-Input.
Used to see how much the client has desynchronized when an accepted Player-Input is received from the server */
USTRUCT()
struct FPositionHistoryElement {
	GENERATED_USTRUCT_BODY()

		/* Current Player-Input-Packet-Number */
		UPROPERTY()
		uint16 PacketNo;

	/* Corresponding Transform of the Pawn in Time when Player-Input with PacketNo was created */
	UPROPERTY()
		FTransform Transform;
};

/* helper struct to calculate velocities of objects, needs to be updated every tick */
struct FVelocity
{
	FVector CurrentLocation;
	FVector PreviousLocation;

	// initializes the values to a location, setting the velocity effectively to zero, prevents large values from occuring on the first read-out
	void Init(const FVector & Location)
	{
		PreviousLocation = Location;
		CurrentLocation = Location;
	}

	/**
	* updates the currentlocation to the new location and stores the current location in the previous location
	*/
	void SetCurrentLocation(const FVector &NewCurrentLocation)
	{
		PreviousLocation = CurrentLocation;
		CurrentLocation = NewCurrentLocation;
	}

	/** calculates the velocity [cm/s] from the current and the previous location and the delta-time 
	* @param DeltaTime	delta-time between the last two Location updates
	*/
	FVector GetVelocityVector(const float DeltaTime)
	{
		// prevent division with zero
		if (DeltaTime == 0.0f) return FVector::ZeroVector;
		return (CurrentLocation - PreviousLocation) / DeltaTime;
	}
};

UCLASS()
class AR_REV_V_CURR_API AMainPawn : public APawn, public ITarget_Interface
{
	GENERATED_BODY()

public:

	/* Interface Implementations: ---------------------------------------
	---------------------------------------------------------------------*/

	/* Implementation of the function to set this pawn to be target-able */
	virtual bool GetIsTargetable_Implementation() override {
		// TODO: when dead -> not target-able
		// if the timer for an active evasive action is active, the pawn is not target-able
		return !GetWorldTimerManager().IsTimerActive(EvasiveActionHandle);
	}

	/*-------------------------------------------------------------------
	 End Interface Implementations--------------------------------------- */


	 /* Initialization Functions: ----------------------------------------
	 ---------------------------------------------------------------------*/

	 // Sets default values for this actor's properties
	AMainPawn(const FObjectInitializer& ObjectInitializer);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/* TODO */
	void InitWeapon();

	/* TODO */
	void InitRadar();

	/* TODO */
	void InitVelocities();

	/* TODO */
	void InitNetwork();

	/*-------------------------------------------------------------------
	End Initialization Functions----------------------------------------- */


	/* TODO */
	void TargetLock();
	void WeaponLock();


	/* Evasive Action Functions: ----------------------------------------
	---------------------------------------------------------------------*/
	// function bound to the activation of the evasive action (e.g. Barrel-Roll-Right)
	void MissileEvasionRight();

	// function bound to the activation of the evasive action (e.g. Barrel-Roll-Left)
	void MissileEvasionLeft();

	/* Function to request an evasive action (e.g. Barrel-Roll);
	takes the roll direction as boolean */
	void RequestEvasiveAction(const bool bDirRight);

	/* Function to request an evasive action (e.g. Barrel-Roll);
	takes the roll direction as boolean, call RequestEvasiveAction(bool) instead of using this function directly */
	UFUNCTION(Server, reliable, WithValidation)
		void Server_RequestEvasiveAction(const bool bDirRight);

	// Handle for the cooldown timer
	FTimerHandle EvasiveActionCoolDown;

	// dummy function that is used for timer to run
	void DoNothing();

	// time in seconds between evasive actions
	float EvasiveActionCoolDownDuration = 3.0f;

	/* called when evasive action is possible and now has to be activated */
	void ActivateEvasiveAction(const bool bDirRight);

	// Handle for the cooldown timer
	FTimerHandle EvasiveActionHandle;

	/*-------------------------------------------------------------------
	End Evasive Action Functions----------------------------------------- */

	// maximum Distance at which the player is able to Lock on to target-able Targets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radar", meta = (ClampMin = "10000.0", ClampMax = "1000000.0", UIMin = "10000.0", UIMax = "1000000.0"))
	float RadarMaxLockOnRange = 165000.0f;
	// squared maximum Distance at which the player is able to Lock on to target-able Targets
	float RadarMaxLockOnRangeSquarred = 0.0f;


	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	/* TODO */
	UFUNCTION()
		void ArmorHit(UPrimitiveComponent * ThisComponent, class AActor* OtherActor, class UPrimitiveComponent * OtherComponent, FVector Loc, const FHitResult& FHitResult);



	/** StaticMesh component that will be the visuals for the missile */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* ArmorMesh;

	/** StaticMesh component that will be the visuals for the missile */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Dummy;

	/* TODO */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;

	/* TODO */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* Camera;

	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return ArmorMesh; }

	/** Returns Root subobject **/
	FORCEINLINE class UStaticMeshComponent* GetDummyMesh() const { return Dummy; }


	/** toogle on screen debug messages */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool DEBUG;

	/** Turnacceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "0.01", ClampMax = "10.0", UIMin = "0.01", UIMax = "10.0"))
		float TurnInterpSpeed = 3.0f;


	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RotControlStrength = 1.0f;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MovControlStrength = 1.0f;



protected:





	//Input variables
	FVector2D CursorLoc;

	/* TODO */
	void GetCursorLocation(FVector2D& CursorLoc);

	/* TODO */
	FVector2D InputAxis;
	/* TODO */
	FVector2D ViewPortSize;
	/* TODO */
	FVector2D ViewPortCenter;

	/* TODO */
	void GetViewportSizeCenter(FVector2D& ViewPortSize, FVector2D& ViewPortCenter);


	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector2D RawTurnInput;

	/* TODO */
	FVector2D MouseInput;

	/* TODO */
	void GetMouseInput(FVector2D& MouseInput, FVector2D& CursorLoc, FVector2D& ViewPortCenter);

	/* TODO */
	FVector2D PreviousMouseInput;


	/* TODO */
	float ForwardVel;
	/* TODO */
	float StrafeVel;
	/* TODO */
	float PrevStrafeRot;
	/* TODO */
	float CurrStrafeRot;
	/* TODO */
	float InputSize;
	/* TODO */
	float NewInputSize;
	/* TODO */
	float OldInputSize;
	/* TODO */
	FVector2D MovementInput;
	/* TODO */
	FVector2D CameraInput;
	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bFreeCameraActive = false;
	/* TODO */
	float ZoomFactor;
	/* TODO */
	uint32 bZoomingIn;
	/* TODO */
	uint32 bBoostPressed;
	/* TODO */
	float SpringArmLength;
	/* TODO */
	FQuat CurrentSpringArmRotation;

	// Weapons 


	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_MainLockOnTarget)
		AActor* MainLockOnTarget;

	// struct object to calculate the velocity of the target
	FVelocity MainTargetVelocity;

	/* TODO */
	UFUNCTION()
		void OnRep_MainLockOnTarget();


	/* TODO */
	TArray<AActor*> MultiTargets;

	/* TODO */
	UFUNCTION(Server, reliable, WithValidation)
		void Server_SetTargets(AActor * MainTarget, const TArray<AActor*> &OtherTargets);

	/* TODO */
	virtual void SetTargets(AActor * MainTarget, const TArray<AActor*> &OtherTargets);

	/* TODO */
	FTimerHandle ContinuousLockOnDelay;
	/* TODO */
	uint32 bLockOnDelayActiv;
	/* TODO */
	uint32 bSwitchTargetPressed;
	/* TODO */
	uint32 bContinuousLockOn;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "10"))
		uint8 MaxNumTargets = 8;

	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_MultiTarget, EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		bool bMultiTarget;
	/* TODO */
	UFUNCTION()
		void OnRep_MultiTarget();
	/* TODO */
	void ActivateContinueousLockOn();

	// Guns ------------------------------------------------------------------------
	/* TODO */
	void StartGunFire();
	/* TODO */
	void StopGunFire();


	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_GunFire)

		/* TODO */
		bool bGunFire;
	/* TODO */
	UFUNCTION()
		void OnRep_GunFire();

	/* TODO */
	UPROPERTY(Replicated)
		uint32 bGunReady : 1;

	/* atempts to start the timer that fires the salves if there is ammunition */
	void GunFire();
	/* TODO */
	FTimerHandle GunFireHandle;

	/* TODO */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapons | Guns")
		void SpawnProjectile(const FTransform &SocketTransform, const bool bTracer, const FVector &FireBaseVelocity = FVector::ZeroVector, const FVector &TracerStartLocation = FVector::ZeroVector);

	/** time in Seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float FireRateGun = 1.0f;

	/* TODO */
	void GunCooldownElapsed();
	/* TODO */
	FTimerHandle GunFireCooldown;

	/* fires a salve with 1..n projectiles */
	void GunFireSalve();
	/* TODO */
	FTimerHandle GunSalveTimerHandle;
	/* TODO */
	float GunSalveIntervall;
	/* TODO */
	uint8 GunCurrentSalve;

	/** number of projectile salves fired after a shot has been triggered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		uint8 GunNumSalves = 4;
	/** smaller values lower the time between the salves and increase the time between last salve and next first salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
		float GunSalveDensity = 1.0f;
	/** number of simultaniously fired projectile in a salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		uint8 NumProjectiles = 2;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		TArray<FName> GunSockets;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float GunRecoilForce = -500000.0f;

	/** number of degrees the projectiles can deviate from actual firedirection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float WeaponSpreadHalfAngle = 0.5f;
	float WeaponSpreadRadian;

	/** number of projectiles available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		int GunAmmunitionAmount = 10000;

	/* TODO */
	UPROPERTY(Replicated)
		bool bGunHasAmmo = true;

	/** every x-th projectile has a tracer, set to 0 to disable tracers completely */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		uint8 TracerIntervall = 1;
	/* TODO */
	uint8 CurrentTracer;
	/* TODO */
	uint8 CurrGunSocketIndex;
	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float ProjectileVel = 100000.0f;

	// Radar  ------------------------------------------------------------------------


	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float MultiTargetLockOnAngleDeg = 30.0f;

	/* TODO */
	float MultiTargetLockOnAngleRad;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float MissileLockOnAngleDeg = 10.0f;

	/* TODO */
	float MissileLockOnAngleRad;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float GunLockOnAngleDeg = 6.0f;

	/* TODO */
	float GunLockOnAngleRad;

	// Missiles -----------------------------------------------------------------------

	/* TODO */
	void StartMissileFire();
	/* TODO */
	void StopMissileFire();

	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_MissileFire)
		bool bMissileFire;
	/* TODO */
	UFUNCTION()
		void OnRep_MissileFire();

	/* TODO */
	UPROPERTY(Replicated)
		uint32 bMissileReady : 1;
	/* TODO */
	void MissileFire();
	/* TODO */
	FTimerHandle MissileFireHandle;

	/* TODO */
	UPROPERTY()
		uint32 bHasMissileLock;

	/* TODO */
	UPROPERTY(Replicated)
		bool bHasGunLock;


	/* TODO */
	int CurrTargetIndex = 0;

	/* TODO */
	UFUNCTION(BlueprintNativeEvent, Category = "Weapons | Missiles")
		void SpawnMissile(const FTransform &SocketTransform, class USceneComponent * HomingTarget, const FVector &FireBaseVelocity = FVector::ZeroVector);

	/** time in Seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		float FireRateMissile = 1.0f;

	/* TODO */
	void MissileCooldownElapsed();
	/* TODO */
	FTimerHandle MissileFireCooldown;

	/* TODO */
	void MissileFireSalve();
	/* TODO */
	FTimerHandle MissileSalveTimerHandle;
	/* TODO */
	float MissileSalveIntervall;
	/* TODO */
	uint8 MissileCurrentSalve;

	/** number of Missile salves fired after a shot has been triggered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		uint8 MissileNumSalves = 4;
	/** smaller values lower the time between the salves and increase the time between last salve and next first salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
		float MissileSalveDensity = 1.0f;
	/** number of fired Missiles in a salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		uint8 NumMissiles = 2;

	/* TODO */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		TArray<FName> MissileSockets;

	/** number of degrees the projectiles can deviate from actual Fire-Direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		float MissileSpreadHalfAngle = 30.0f;

	/* TODO */
	float MissileSpreadRadian;

	/** number of projectiles available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		int MissileAmmunitionAmount = 10000;
	/* TODO */
	UPROPERTY(Replicated)
		bool bMissileHasAmmo = true;

	/* TODO */
	uint8 CurrMissileSocketIndex;

	// END Weapons ------------------------------------------------------------------------

	//Input functions
	/* TODO */
	void MoveForward(float AxisValue);
	/* TODO */
	void MoveRight(float AxisValue);
	/* TODO */
	void PitchCamera(float AxisValue);
	/* TODO */
	void YawCamera(float AxisValue);
	/* TODO */
	void ActivateFreeCamera();
	/* TODO */
	void DeactivateFreeCamera();
	/* TODO */
	void ZoomIn();
	/* TODO */
	void ZoomOut();
	/* TODO */
	void SwitchTargetPressed();
	/* TODO */
	void SwitchTargetReleased();

	/* TODO */
	void StartBoost();
	/* TODO */
	void StopBoost();

	/* TODO */
	void Skill_01_Pressed();
	void Skill_02_Pressed();
	void Skill_03_Pressed();
	void Skill_04_Pressed();
	void Skill_05_Pressed();
	void Skill_06_Pressed();
	void Skill_07_Pressed();
	void Skill_08_Pressed();
	void Skill_09_Pressed();
	void Skill_10_Pressed();

	void Skill_01_Released();
	void Skill_02_Released();
	void Skill_03_Released();
	void Skill_04_Released();
	void Skill_05_Released();
	void Skill_06_Released();
	void Skill_07_Released();
	void Skill_08_Released();
	void Skill_09_Released();
	void Skill_10_Released();

	/* TODO */
	void StopMovement();
	/* TODO */
	UFUNCTION(Server, reliable, WithValidation)
		void Server_StopPlayerMovement();
	/* TODO */
	virtual void StopPlayerMovement();

	/* TODO */
	UPROPERTY(Replicated)
		bool bCanReceivePlayerInput;
	/* TODO */
	UFUNCTION()
		void StartMovementCoolDownElapsed();
	/* TODO */
	FTimerHandle StartMovementTimerHandle;

	/* TODO */
	int lagCounter = 0;

	/* TODO */
	FTransform RelativeArmorTransform;

	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite)
		FVector AngularVelocity;
	/* TODO */
	UFUNCTION()
		void OnRep_AngularVelocity();

	/* TODO */
	FVector WorldAngVel;
	/* TODO */
	FVector TargetLinearVelocity;
	/* TODO */
	float TransformBlend;

	/* TODO */
	UFUNCTION(Server, unreliable, WithValidation)
		void Server_GetPlayerInput(FPlayerInputPackage inputData);
	/* TODO */
	virtual void GetPlayerInput(FPlayerInputPackage inputData);


	/* TODO */
	UPROPERTY()
		FPlayerInputPackage InputPackage;
	/* TODO */
	UPROPERTY()
		uint32 Ack;
	/* TODO */
	UPROPERTY(ReplicatedUsing = OnRep_AuthorityAck)
		uint32 AuthorityAck;
	/* TODO */
	UFUNCTION()
		void OnRep_AuthorityAck();
	/* TODO */
	FTransform PastClientTransform;
	/* TODO */
	FVector LocationCorrection;


	/* TODO */
	UPROPERTY()
		TArray<FPositionHistoryElement> MovementHistory;

	// Replicated Movement
	// {
	/** Smoothing defines how many seconds the client will continue to move after an update was received */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication", meta = (ClampMin = "0.0", ClampMax = "3.0", UIMin = "0.0", UIMax = "1.0"))
		float Smoothing = 0.5f;

	/** defines how many updates will be predicted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication", meta = (ClampMin = "-1", ClampMax = "100", UIMin = "-1", UIMax = "3"))
		int AdditionalUpdatePredictions = -1;



	/** max Turn-Rate in deg/second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "40.0", ClampMax = "360.0", UIMin = "40.0", UIMax = "180.0"))
		float MaxTurnRate = 50.0f;

	/** Dead-Zone area with no turning, factor is percentage of screen width in range of 0 to 1 (100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "0.0", ClampMax = "0.1", UIMin = "0.0", UIMax = "0.05"))
		float Deadzone = 0.005f;
	/** how long the player has no control after a crash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.001", ClampMax = "10.0", UIMin = "0.001", UIMax = "1.0"))
		float TimeOfNoControl = 1.0f;
	/** how long the player has no control after a crash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.001", ClampMax = "10.0", UIMin = "0.001", UIMax = "1.0"))
		float TimeOfAntiCollisionSystem = 3.0f;
	/** default Velocity when input is zero */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float DefaultForwardVel = 5000.0f;
	/** max flight velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxVelocity = 16000.0f;
	/** minimum velocity, negative values result in backwards flight [cm/s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MinVelocity = 2500.0f;
	float VelForwardDelta;
	float VelBackwardsDelta;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float CurrentVelocitySize;
	/** max velocity to the right and to the left */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxStrafeVel = 3000.0f;
	/** Interp-velocity for forward movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float ForwardAcceleration = 2.0f;
	/** Interp-velocity for backwards movement/braking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float BackwardsAcceleration = 3.0f;
	/** if false  strafe input causes instant change to MaxStrafeVel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		bool bUseConstantStrafeAcceleration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.001", ClampMax = "10.0", UIMin = "0.001", UIMax = "1.0"))
		float TimeToMaxStrafeVel = 0.15f;
	float ConstantStrafeAcceleration;

	/** Interp-velocity for side movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float StrafeBankAcceleration = 2.0f;

	/** Strafe-Rotation angle in range of -72 to 72 deg (roll) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
		float MaxStrafeBankAngle = 72.0f;

	/** how fast the roll component of the actor current rotation is compensated for; set to 0 to deactivate autolevel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "10.0"))
		float LevelVel = 3.0f;
	/** Axis which is used to level the aircraft, e.g. if there was a planet with gravity: it is the vector pointing from its center towards the aircraft (up) */
	/** has to be normalized! */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
		FVector AutoLevelAxis = FVector(0.0f, 0.0f, 1.0f);

	/** use the gravity of planets to determine what direction is or use world Up-Vector when set to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls")
		bool bUseGravityDirForAutoLevel = true;

	/** factor to speed up Free-Look camera  rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "720.0", UIMin = "0.0", UIMax = "100.0"))
		float FreeCameraSpeed = 5.0f;

	/** true: use separate mouseinput in combination with MouseSensitivity (better for widescreen displays); false: use the cursorposition inside game (same sensitivity as Windows, slightly higher inputlag); regardless of which method choosen: the max turnrates are the same */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls")
		bool bUseInternMouseSensitivity = true;
	/** how Sensitivity direction control is, only used in when UseInternMouseSensitivity is active, otherwise the Cursor-Position on screen is used which uses the Windows sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseSensitivity = 0.01f;
	/** CenterPrecision defines how sensitive the turning is around the center of the screen, towards 0: linear turning, towards 1: [1.0 - cos(x * CP * pi/2)] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float CenterPrecision = 0.25f;
	/** factor to decrease the time it takes to stop turning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "1.0", ClampMax = "100.0", UIMin = "1.0", UIMax = "10.0"))
		float ResetSpeed = 5.0f;



private:

	/* TODO */
	void MainPlayerMovement(const float DeltaTime, const FVector &CorrectionVelocity = FVector::ZeroVector, const FVector &CorrectionAngularVelocity = FVector::ZeroVector);


	// how many updates to buffer
	int NumberOfBufferedNetUpdates;
	// factor to slow down movement to compensate long Buffer-Time
	float LerpVelocity;
	// how far the received Location will be predicted into the future
	float PredictionAmount;
	// how far the client has transitioned between the current transform and the target transform */
	float LerpProgress;
	// start transform used for lerp-movement */
	FTransform TransformOnClient;
	// end transform used for lerp-movement */
	FTransform TargetTransform;

	/** last Transform received by the client/sent by the server/authority */
	UPROPERTY(ReplicatedUsing = OnRep_TransformOnAuthority)
		FTransform TransformOnAuthority;
	UFUNCTION()
		void OnRep_TransformOnAuthority();
	FVector LinVelError;
	FVector AngVelError;

	/**  last linear physics velocity by the client/sent by the server/authority */
	UPROPERTY(ReplicatedUsing = OnRep_LinearVelocity)
		FVector LinearVelocity;
	UFUNCTION()
		void OnRep_LinearVelocity();



	/* TODO */
	UFUNCTION()
		void RecoverFromCollision(const float DeltaSeconds);

	/* TODO */
	FVector MostRecentCrashPoint;
	/* TODO */
	FVector CrashNormal;
	/* TODO */
	float CollisionTimeDelta;
	/* TODO */
	float PrevSafetyDistanceDelta;
	/* TODO */
	float PrevDeltaTime = 1.0f;
	/* TODO */
	float AntiCollisionVelocity;
	/* TODO */
	bool CollisionHandling;

	// }


	// general {
	// PI / 2
	const float HalfPI = 0.5f * PI;
	// }
	/* TODO */
	float lastUpdate;
	/* TODO */
	FVector PrevLocationOnServer;
	/* TODO */
	FTransform PrevReceivedTransform;
	/* TODO */
	float LinVelServer;
	/* TODO */
	float NetDelta;

	// Player-State
	UPROPERTY()
		APlayerState* State;

	// network
	UFUNCTION()
		void GetPing();
	/* TODO */
	float Ping;
	/* TODO */
	float Alpha;
};

