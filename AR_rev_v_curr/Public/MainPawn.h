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

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class DebugTurning : uint8
{
	Default 	        UMETA(DisplayName = "Default"),
	SmoothWithFastStop 	UMETA(DisplayName = "SmoothWithFastStop"),
	Smooth	            UMETA(DisplayName = "Smooth")
};

USTRUCT()
struct FPlayerInputPackage {
	GENERATED_USTRUCT_BODY()
		UPROPERTY()
		uint16 PacketNo;

	UPROPERTY()
		FVector2D MouseInput;

	// 0000 backwards forward left right
	UPROPERTY()		
		uint8 MovementData;

	/*UPROPERTY()
		uint64 PlayerDataCompressed;*/

	void SetMovementInput(const FVector2D &MovementInput) {
		MovementData = 0;
		// forward
		if (MovementInput.X > 0.0f) {
			MovementData |= 1 << 2;
		}
		// backwards
		if (MovementInput.X < 0.0f) {
			MovementData |= 1 << 3;
		}
		// right
		if (MovementInput.Y > 0.0f) {
			MovementData |= 1 << 0;
		}
		// left
		if (MovementInput.Y < 0.0f) {
			MovementData |= 1 << 1;
		}
	}

	FVector2D GetMovementInput() {
		FVector2D Input;
		// forward
		if (MovementData & 1 << 2) {
			Input.X = 1.0f;
		}
		// backwards
		if (MovementData & 1 << 3) {
			Input.X = -1.0f;
		}
		// right
		if (MovementData & 1 << 0) {
			Input.Y = 1.0f;
		}
		// left
		if (MovementData & 1 << 1) {
			Input.Y = -1.0f;
		}
		return Input;
	}
};


USTRUCT()
struct FPositionHistoryElement {
	GENERATED_USTRUCT_BODY()
		UPROPERTY()
		uint16 PacketNo;
	UPROPERTY()
		FTransform Transform;
};


UCLASS()
class AR_REV_V_CURR_API AMainPawn : public APawn, public ITarget_Interface
{
	GENERATED_BODY()

public:

	virtual bool GetIsTargetable_Implementation() override {
		return true;
	}

	// Sets default values for this pawn's properties
	AMainPawn();

	// Sets default values for this actor's properties
	AMainPawn(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
		void ArmorHit(UPrimitiveComponent * ThisComponent, class AActor* OtherActor, class UPrimitiveComponent * OtherComponent, FVector Loc, const FHitResult& FHitResult);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** StaticMesh component that will be the visuals for the missile */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* ArmorMesh;

	/** StaticMesh component that will be the visuals for the missile */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Dummy;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;
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


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RotControlStrength = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MovControlStrength = 1.0f;



protected:





	//Input variables
	FVector2D CursorLoc;
	void GetCursorLocation(FVector2D& CursorLoc);

	FVector2D InputAxis;
	FVector2D ViewPortSize;
	FVector2D ViewPortCenter;
	void GetViewportSizeCenter(FVector2D& ViewPortSize, FVector2D& ViewPortCenter);

	FVector2D MouseInput;
	void GetMouseInput(FVector2D& MouseInput, FVector2D& CursorLoc, FVector2D& ViewPortCenter);

	FVector2D PreviousMouseInput;


	float ForwardVel;
	float StrafeVel;
	float PrevStrafeRot;
	float CurrStrafeRot;
	float InputSize;
	float NewInputSize;
	float OldInputSize;
	FVector2D MovementInput;
	FVector2D CameraInput;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool bFreeCameraActive = false;
	float ZoomFactor;
	uint32 bZoomingIn;
	uint32 bBoostPressed;
	float SpringArmLength;
	FQuat CurrentSpringArmRotation;

	// Weapons 
	void InitWeapon();
	void TargetLock();





	AActor* CurrLockOnTarget;
	TArray<AActor*> MultiTargets;
	UFUNCTION(Server, reliable, WithValidation)
		void Server_SetTargets(AActor * MainTarget, const TArray<AActor*> &OtherTargets);
	virtual void SetTargets(AActor * MainTarget, const TArray<AActor*> &OtherTargets);
	FTimerHandle ContinuousLockOnDelay;
	uint32 bLockOnDelayActiv;
	uint32 bSwitchTargetPressed;
	uint32 bContinuousLockOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "10"))
		uint8 MaxNumTargets = 8;

	UPROPERTY(ReplicatedUsing = OnRep_MultiTarget, EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		bool bMultiTarget;
	UFUNCTION()
		void OnRep_MultiTarget();
	void ActivateContinueousLockOn();

	// Guns ------------------------------------------------------------------------
	void StartGunFire();
	void StopGunFire();
	uint32 bGunFire;
	UPROPERTY(Replicated)
		uint32 bGunReady : 1;
	void GunFire();
	FTimerHandle GunFireHandle;

	UFUNCTION(BlueprintNativeEvent, Category = "Weapons | Guns")
		void SpawnProjectile(const FTransform &SocketTransform, const bool bTracer, const FVector &FireBaseVelocity = FVector::ZeroVector, const FVector &TracerStartLocation = FVector::ZeroVector);

	/** time in Seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float FireRateGun = 1.0f;

	void GunCooldownElapsed();
	FTimerHandle GunFireCooldown;

	void GunFireSalve();
	FTimerHandle GunSalveTimerHandle;
	float GunSalveIntervall;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		TArray<FName> GunSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float GunRecoilForce = -500000.0f;

	/** number of degrees the projectiles can deviate from actual firedirection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float WeaponSpreadHalfAngle = 0.5f;
	float WeaponSpreadRadian;

	/** number of projectiles available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		int GunAmmunitionAmount = 10000;
	UPROPERTY(Replicated)
		bool bGunHasAmmo = true;

	/** every x-th projectile has a tracer, set to 0 to disable tracers completely */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		uint8 TracerIntervall = 1;
	uint8 CurrentTracer;
	uint8 CurrGunSocketIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Guns")
		float ProjectileVel = 100000.0f;

	// Radar  ------------------------------------------------------------------------
	void InitRadar();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float MultiTargetLockOnAngleDeg = 30.0f;
	float MultiTargetLockOnAngleRad;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float MissileLockOnAngleDeg = 10.0f;
	float MissileLockOnAngleRad;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "90.0"))
		float GunLockOnAngleDeg = 6.0f;
	float GunLockOnAngleRad;

	// Missiles -----------------------------------------------------------------------
	void StartMissileFire();
	void StopMissileFire();
	uint32 bMissileFire;
	UPROPERTY(Replicated)
		uint32 bMissileReady : 1;
	void MissileFire();
	FTimerHandle MissileFireHandle;

	UFUNCTION(BlueprintNativeEvent, Category = "Weapons | Missiles")
		void SpawnMissile(const FTransform &SocketTransform, class USceneComponent * HomingTarget, const FVector &FireBaseVelocity = FVector::ZeroVector);

	/** time in Seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		float FireRateMissile = 1.0f;

	void MissileCooldownElapsed();
	FTimerHandle MissileFireCooldown;

	void MissileFireSalve();
	FTimerHandle MissileSalveTimerHandle;
	float MissileSalveIntervall;
	uint8 MissileCurrentSalve;

	/** number of Missile salves fired after a shot has been triggered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		uint8 MissileNumSalves = 4;
	/** smaller values lower the time between the salves and increase the time between last salve and next first salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
		float MissileSalveDensity = 1.0f;
	/** number of simultaniously fired Missiles in a salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		uint8 NumMissiles = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		TArray<FName> MissileSockets;

	/** number of degrees the projectiles can deviate from actual firedirection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		float MissileSpreadHalfAngle = 30.0f;
	float MissileSpreadRadian;

	/** number of projectiles available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons | Missiles")
		int MissileAmmunitionAmount = 10000;
	UPROPERTY(Replicated)
		bool bMissileHasAmmo = true;

	uint8 CurrMissileSocketIndex;

	// END Weapons ------------------------------------------------------------------------

	//Input functions
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void PitchCamera(float AxisValue);
	void YawCamera(float AxisValue);
	void ActivateFreeCamera();
	void DeactivateFreeCamera();
	void ZoomIn();
	void ZoomOut();
	void SwitchTargetPressed();
	void SwitchTargetReleased();


	void StartBoost();
	void StopBoost();

	void StopMovement();
	UFUNCTION(Server, reliable, WithValidation)
		void Server_StopPlayerMovement();
	virtual void StopPlayerMovement(); // executed on client
	UPROPERTY(Replicated)
		bool bCanReceivePlayerInput;
	UFUNCTION()
		void StartMovementCoolDownElapsed();
	FTimerHandle StartMovementTimerHandle;

	int lagCounter = 0;

	FTransform RelativeArmorTransform;

	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite)
		FVector AngularVelocity;
	UFUNCTION()
		void OnRep_AngularVelocity();

	FVector WorldAngVel;
	FVector TargetLinearVelocity;
	float TransformBlend;

	UFUNCTION(Server, unreliable, WithValidation)
		void Server_GetPlayerInput(FPlayerInputPackage inputData);
	virtual void GetPlayerInput(FPlayerInputPackage inputData); // executed on client

	
	UPROPERTY()
		FPlayerInputPackage InputPackage;
	UPROPERTY()
		uint16 Ack;
	UPROPERTY(ReplicatedUsing = OnRep_AuthorityAck)
		uint16 AuthorityAck;
	UFUNCTION()
		void OnRep_AuthorityAck();
	FTransform PastClientTransform;
	FVector LocationCorrection;


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



	/** max turnrate in deg/second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "40.0", ClampMax = "360.0", UIMin = "40.0", UIMax = "180.0"))
		float MaxTurnRate = 50.0f;

	/** deadzone area with no turning, factor is percentage of screen width in range of 0 to 1 (100%) */
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
	UFUNCTION()
		void InitVelocities();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float CurrentVelocitySize;
	/** max velocity to the right and to the left */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxStrafeVel = 3000.0f;
	/** interpvelocity for forward movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float ForwardAcceleration = 2.0f;
	/** interpvelocity for backwards movement/braking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float BackwardsAcceleration = 3.0f;
	/** if false  strafe input causes instant change to MaxStrafeVel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		bool bUseConstantStrafeAcceleration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.001", ClampMax = "10.0", UIMin = "0.001", UIMax = "1.0"))
		float TimeToMaxStrafeVel = 0.15f;
	float ConstantStrafeAcceleration;

	/** interpvelocity for side movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float StrafeBankAcceleration = 2.0f;

	/** straferotation angle in range of -72 to 72 deg (roll) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
		float MaxStrafeBankAngle = 72.0f;

	/** how fast the roll component of the actor current rotation is compensated for; set to 0 to deactivate autolevel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "10.0"))
		float LevelVel = 3.0f;
	/** Axis which is used to level the aircraft, e.g. if there was a planet with gravity: it is the vector pointing from its center towards the aircraft (up) */
	/** has to be normalized! */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
		FVector AutoLevelAxis = FVector(0.0f, 0.0f, 1.0f);

	/** use the gravity of planets to determine what direction is or use world upvector when set to false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls")
		bool bUseGravityDirForAutoLevel = true;

	/** factor to speed up freelook camera  rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "720.0", UIMin = "0.0", UIMax = "100.0"))
		float FreeCameraSpeed = 5.0f;

	/** true: use separate mouseinput in combination with MouseSensitivity (better for widescreen displays); false: use the cursorposition inside game (same sensitivity as Windows, slightly higher inputlag); regardless of which method choosen: the max turnrates are the same */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls")
		bool bUseInternMouseSensitivity = true;
	/** how sensitiv direction control is, only used in when UseInternMouseSensitivity is activ, otherwise the cursorposition on screen is used which uses the Windows sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float MouseSensitivity = 0.01f;
	/** CenterPrecision defines how sensitive the turning is around the center of the screen, towards 0: linear turning, towards 1: [1.0 - cos(x * CP * pi/2)] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float CenterPrecision = 0.25f;
	/** factor to decrease the time it takes to stop turning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "1.0", ClampMax = "100.0", UIMin = "1.0", UIMax = "10.0"))
		float ResetSpeed = 5.0f;



private:

	void MainPlayerMovement(const float DeltaTime, const FVector &CorrectionVelocity = FVector::ZeroVector, const FVector &CorrectionAngularVelocity = FVector::ZeroVector);

	void InitNetwork();
	// how many updates to buffer
	int NumberOfBufferedNetUpdates;
	// factor to slow down movement to compensate long buffertime
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



	UFUNCTION()
		void RecoverFromCollision(const float DeltaSeconds);

	FVector MostRecentCrashPoint;
	FVector CrashNormal;
	float CollisionTimeDelta;
	float PrevSafetyDistanceDelta;
	float PrevDeltaTime = 1.0f;
	float AntiCollisionVelocity;
	bool CollisionHandling;

	// }


	// general {
	// PI / 2
	const float HalfPI = 0.5f * PI;
	// }
	float lastUpdate;
	FVector PrevLocationOnServer;
	FTransform PrevReceivedTransform;
	float LinVelServer;
	float NetDelta;

	// playerstate
	UPROPERTY()
		APlayerState* State;

	// network
	UFUNCTION()
		void GetPing();
	float Ping;
	float Alpha;
};
