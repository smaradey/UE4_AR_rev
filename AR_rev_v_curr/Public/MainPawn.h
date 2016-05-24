// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "AR_rev_v_curr.h"
#include "GameFramework/Pawn.h"
#include "Engine.h"
#include "GameFramework/DamageType.h"
#include "UnrealNetwork.h"
#include "MainPawn.generated.h"

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class DebugTurning : uint8
{
	Default 	        UMETA(DisplayName = "Default"),
	SmoothWithFastStop 	UMETA(DisplayName = "SmoothWithFastStop"),
	Smooth	            UMETA(DisplayName = "Smooth")
};

USTRUCT()
struct FInput {
	GENERATED_USTRUCT_BODY()
		UPROPERTY()
		int16 PacketNo;
	UPROPERTY()
		FVector2D MouseInput;
	UPROPERTY()
		FVector2D MovementInput;
};

USTRUCT()
struct FInputsPackage {
	GENERATED_USTRUCT_BODY()
		UPROPERTY()
		int16 PacketNo;
	UPROPERTY()
		int16 Ack;
	UPROPERTY()
		TArray<FInput> InputDataList = TArray<FInput>();
};

UCLASS()
class AR_REV_V_CURR_API AMainPawn : public APawn
{
	GENERATED_BODY()

public:


	// Sets default values for this pawn's properties
	AMainPawn();

	// Sets default values for this actor's properties
	AMainPawn(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

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
	void StartGunFire();
	void StopGunFire();
	uint32 bGunFire;
	UPROPERTY(Replicated)
		uint32 bGunReady : 1;


	void GunFire();
	FTimerHandle GunFireHandle;
	/** time in Seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		float FireRateGun = 1.0f;

	void GunCooldownElapsed();
	FTimerHandle GunFireCooldown; 

	void FireSalve();
	FTimerHandle SalveTimerHandle;
	float SalveIntervall;
	uint8 CurrentSalve;
	/** number of projectile salves fired after a shot has been triggered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		uint8 NumSalves = 4;
	/** smaller values lower the time between the salves and increase the time between last salve and next first salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons", meta = (ClampMin = "0.05", ClampMax = "1.0", UIMin = "0.05", UIMax = "1.0"))
		float SalveDensity = 1.0f;
	/** number of simultaniously fired projectile in a salve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		uint8 NumProjectiles = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		TArray<FName> GunSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		float GunRecoilForce = -500000.0f;

	/** number of degrees the projectiles can deviate from actual firedirection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		float WeaponSpreadHalfAngle = 0.5f;
	float WeaponSpreadRadian;

	/** number of projectiles available */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		int GunAmmunitionAmount = 10000;
	bool bHasAmmo = true;

	/** every x-th projectile has a tracer, set to 0 to disable tracers completely */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
		uint8 TracerIntervall = 1;
	uint8 CurrentTracer;
	uint8 CurrGunSocketIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	float ProjectileVel = 100000.0f;
	// END Weapons

	//Input functions
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void PitchCamera(float AxisValue);
	void YawCamera(float AxisValue);
	void ActivateFreeCamera();
	void DeactivateFreeCamera();
	void ZoomIn();
	void ZoomOut();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapons")
		void SpawnProjectile(const FTransform &SocketTransform,const bool bTracer, const FVector &FireBaseVelocity = FVector::ZeroVector, const FVector &TracerStartLocation = FVector::ZeroVector);
	void StartBoost();
	void StopBoost();

	void StopMovement();
	UFUNCTION(Server, reliable, WithValidation)
		void Server_StopPlayerMovement();
	virtual void StopPlayerMovement(); // executed on client
	UPROPERTY(Replicated)
		bool bCanReceivePlayerInput = true;
	UFUNCTION()
		void StartMovementCoolDownElapsed();
	FTimerHandle StartMovementTimerHandle;

	int lagCounter = 0;

	FTransform RelativeArmorTransform;

	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite)
		FVector AngularVelocity;
	UFUNCTION()
		void OnRep_AngularVelocity();

	UPROPERTY(Replicated)
		FVector WorldAngVel;

	UPROPERTY(Replicated)
		FVector TargetLinearVelocity;
	float TransformBlend;

	UFUNCTION(Server, unreliable, WithValidation)
		void Server_GetPlayerInput(FInputsPackage inputData);
	virtual void GetPlayerInput(FInputsPackage inputData); // executed on client

	UFUNCTION(Client, reliable)
		void Client_LastAcceptedPacket(int16 Ack);
	virtual void LastAcceptedPacket(int16 Ack);
	UPROPERTY()
		TArray<FInput> PlayerInputs = TArray<FInput>();
	UPROPERTY()
		FInputsPackage InputPackage;
	UPROPERTY()
		int16 Ack;

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
	void CalculateVelocityDeltas();
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
	/** interpvelocity for side movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float StrafeAcceleration = 2.0f;

	/** straferotation angle in range of -72 to 72 deg (roll) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
		float MaxStrafeBankAngle = 72.0f;

	/** how fast the roll component of the actor current rotation is compensated for; set to 0 to deactivate autolevel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerControls", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "10.0"))
		float LevelVel = 3.0f;
	/** Axis which is used to level the aircraft, e.g. if there was a planet with gravity: it is the vector pointing from its center towards the aircraft (up) */
	/** has to be normalized! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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

	/**  last linear physics velocity by the client/sent by the server/authority */
	UPROPERTY(ReplicatedUsing = OnRep_LinearVelocity)
		FVector LinearVelocity;
	UFUNCTION()
		void OnRep_LinearVelocity();




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
