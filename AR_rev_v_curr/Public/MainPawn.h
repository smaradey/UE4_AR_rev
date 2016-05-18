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
		bool DEBUG = false;
	/** switch between turn implementations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bUseSmoothedTurning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enum)
		DebugTurning TurnOption;

	/** Turnacceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "0.01", ClampMax = "10.0", UIMin = "0.01", UIMax = "10.0"))
		float TurnInterpSpeed = 1.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RotControlStrength = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MovControlStrength = 1.0f;



protected:





	//Input variables
	FVector2D CursorLoc;
	void GetCursorLocation(FVector2D& CursorLoc);

	FVector2D ViewPortSize;
	FVector2D ViewPortCenter;
	void GetViewportSizeCenter(FVector2D& ViewPortSize, FVector2D& ViewPortCenter);

	FVector2D MouseInput;
	void GetMouseInput(FVector2D& MouseInput, FVector2D& CursorLoc, FVector2D& ViewPortCenter);

	FVector2D PreviousMouseInput;

	float ForwardVel;
	float StrafeVel;
	float PrevStrafeRot;

	float InputSize;
	float NewInputSize;
	float OldInputSize;
	FVector2D MovementInput;
	FVector2D CameraInput;
	float ZoomFactor;
	bool bZoomingIn;

	//Weapons
	bool bGunFire;
	UPROPERTY(Replicated)
		bool bCanFireGun = true;
	float FireRateGun = 0.1f;

	//TimerHandle
	UPROPERTY()
		FTimerHandle GunFireHandle;

	//Input functions
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void PitchCamera(float AxisValue);
	void YawCamera(float AxisValue);
	void ZoomIn();
	void ZoomOut();
	void GunFire();
	void StartGunFire();
	void StopGunFire();

	void StopMovement();
	UFUNCTION(Server, reliable, WithValidation)
		void Server_StopPlayerMovement();
	virtual void StopPlayerMovement(); // executed on client
	UPROPERTY(Replicated)
		bool bCanReceivePlayerInput = true;

	int lagCounter = 0;

	FTransform RelativeArmorTransform;

	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite, Category = "MainPawn")
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

	/** CenterPrecision defines how sensitive the turning is around the center of the screen, towards 0: linear turning, towards 1: [1.0 - cos(x * CP * pi/2)] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float CenterPrecision = 0.5f;

	/** max turnrate in deg/second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "40.0", ClampMax = "360.0", UIMin = "40.0", UIMax = "180.0"))
		float MaxTurnRate = 50.0f;

	/** deadzone area with no turning, factor is percentage of screen width in range of 0 to 1 (100%) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turning", meta = (ClampMin = "0.0", ClampMax = "0.1", UIMin = "0.0", UIMax = "0.05"))
		float Deadzone = 0.01f;

		/** default Velocity when input is zero */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float DefaultForwardVel = 5000.0f;
		/** max flight velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxForwardVel = 50000.0f;
		/** max backwards velocity, resulting velocity is (DefaultForwardVel - MaxBackwardsVel) e.g. 5000.0 - 5000.0 = 0.0 [cm/s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxBackwardsVel = 5000.0f;
		/** max velocity to the right and to the left */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		float MaxStrafeVel = 10000.0f;

	/** how fast the roll component of the actor current rotation is compensated for; set to 0 to deactivate autolevel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "10.0"))
		float LevelVel = 2.0f;
	/** Axis which is used to level the aircraft, e.g. if there was a planet with gravity: it is the vector pointing from its center towards the aircraft (up) */
	/** has to be normalized! */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
		FVector AutoLevelAxis = FVector(0.0f,0.0f,1.0f);

	/** straferotation angle in range of -72 to 72 deg (roll) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
		float MaxStrafeBankAngle = 72.0f;

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
