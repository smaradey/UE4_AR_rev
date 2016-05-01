// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "AR_rev_v_curr.h"
#include "GameFramework/Pawn.h"
#include "Engine.h"
#include "GameFramework/DamageType.h"
#include "UnrealNetwork.h"
#include "MainPawn.generated.h"



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

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* SpringArm;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* Camera;

	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return ArmorMesh; }







protected:





	//Input variables
	FVector2D CursorLoc;
	void GetCursorLocation(FVector2D& CursorLoc);

	FVector2D ViewPortSize;
	FVector2D ViewPortCenter;
	void GetViewportSizeCenter(FVector2D& ViewPortSize, FVector2D& ViewPortCenter);

	FVector2D MouseInput;
	void GetMouseInput(FVector2D& MouseInput, FVector2D& CursorLoc, FVector2D& ViewPortCenter);

	FVector2D OldMouseInput;
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

	int lagCounter = 0;




	FTransform RelativeArmorTransform;



	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite, Category = "MainPawn")
		FVector AngularVelocity;
	UFUNCTION()
		void OnRep_AngularVelocity();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float MaxTurnRate = 100.0f;
	float TurnRate;

	UPROPERTY(Replicated)
		FVector TargetAngularVelocity;

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
