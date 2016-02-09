// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "GameFramework/Pawn.h"
#include "Engine.h"
#include "GameFramework/DamageType.h"
#include "UnrealNetwork.h"
#include "MainPawn.generated.h"

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

	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return ArmorMesh; }



protected:
	UPROPERTY(EditAnywhere)
		USpringArmComponent* OurCameraSpringArm;
	UCameraComponent* OurCamera;

	//Input variables
	FVector2D CursorLoc;
	FVector2D ViewPortSize;
	FVector2D ViewPortCenter;
	FVector2D MouseInput;
	FVector2D OldMouseInput;
	float InputSize;
	float NewInputSize;
	float OldInputSize;
	FVector2D MovementInput;
	FVector2D CameraInput;
	float ZoomFactor;
	bool bZoomingIn;

	/** missile velocity in cm/s*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Missile")
		float MaxTurnRate = 100.0f;
	float TurnRate;



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

	// movement
	UPROPERTY(ReplicatedUsing = OnRep_TransformOnAuthority, EditAnywhere, BlueprintReadWrite, Category = "MainPawn")
		FTransform TransformOnAuthority;
	UFUNCTION()
		void OnRep_TransformOnAuthority();

	FTransform TransformOnClient;
	FTransform RelativeArmorTransform;

	UPROPERTY(ReplicatedUsing = OnRep_LinearVelocity, EditAnywhere, BlueprintReadWrite, Category = "MainPawn")
		FVector LinearVelocity;
	UFUNCTION()
		void OnRep_LinearVelocity();

	UPROPERTY(ReplicatedUsing = OnRep_AngularVelocity, EditAnywhere, BlueprintReadWrite, Category = "MainPawn")
		FVector AngularVelocity;
	UFUNCTION()
		void OnRep_AngularVelocity();




	// playerstate
	UPROPERTY()
		APlayerState* State;

	// network
	UFUNCTION()
		void GetPing();
	float Ping;
	float Alpha;
};
