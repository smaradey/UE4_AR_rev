// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "Turret.generated.h"

UENUM(BlueprintType)
enum class ETurretOperationMode : uint8
{
	Rest		UMETA(DisplayName = "Rest"),
	Track 		UMETA(DisplayName = "Track"),
	AimOnce	    UMETA(DisplayName = "AimOnce"),
	Freeze		UMETA(DisplayName = "Freeze")
};

UENUM(BlueprintType)
enum class ETurretRotationOrder : uint8
{
	Simultaneously		UMETA(DisplayName = "Simultaneously"),
	YawPitch 			UMETA(DisplayName = "YawPitch"),
	PitchYaw			UMETA(DisplayName = "PitchYaw")
};

UCLASS()
class AR_REV_V_CURR_API ATurret : public AActor
{
	GENERATED_BODY()

public:

	// Sets default values for this actor's properties
	ATurret(const FObjectInitializer& ObjectInitializer);

	// called in editor for calculation of missing values with dependencies
	void PostInitProperties();
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// get all sockets of the pitch-part starting with "muzzle"
	UFUNCTION(BlueprintCallable, Category = "Turret")
	void GetProjectileSpawnPointSockets();

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// manually Calculate this Actors Velocity in Worldspace, call once every Tick.
	inline void CalcTurretWorldVelocityVector(const float& DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Turret")
		void UpdateTurret(const FVector& TargetedLocation, const ETurretOperationMode& NewOperationMode);

	// fire the turrets Weapon
	// @Param bAcceptInaccuracy true: always fire, false: fire only if the turret has finished aiming and is not resting.
	// @Param Accuracy Deltaangle to use when bAcceptInaccuracy is false
	UFUNCTION(BlueprintCallable, Category = "Turret")
		bool Fire(const bool bAcceptInaccuracy, const float Accuracy = 0.01f);

	UFUNCTION(BlueprintNativeEvent, Category = "Turret|Fire")
		void StartFireing();

	/* Set the AimLocation to be default Forward */
	void SetRestingAimLocation();

	/* Calculate the target rotation to be able to hit the location/target */
	void GetAimDirection(const FVector & TargetLocation);

	/* Calculate The Relative Rotation of the Pitch-Part (Barrels relative to Root) */
	void CalculateCurrentRelativeAimRotation();

	// Helper function that combines 3 function calls
	inline void CalculateAndChooseRotation(const float& DeltaTime);

	void CalculateConstantYawRotation(const float& DeltaTime);
	void CalculateConstantPitchRotation(const float& DeltaTime);	
	void ChooseRotations(const float& DeltaTime);
	inline FRotator CalcFinalRotation(const float& DeltaTime);

	/* Rotates the turret using the choosen Rotation */
	void RotateTurret();

	/* Checks whether the Turret is aiming at the Target using a default precision of 1/1000th of a degree */
	bool CheckAimFinished(const float& Precision = 0.001f) const;

	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Root;
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* TurretYawPart;
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* TurretPitchPart;

	// Socketname of the Rootcomponent where the yaw Rotation component will be attached to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
		FName RootConnectionSocket = "None";

	// Socketname of the yaw Rotation component where the pitch Rotation component will be attached to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
		FName YawPartConnectionSocket = "pitch";

	// Prefix of the socketnames (e.g. muzzle -> muzzle_01, muzzle_02,...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
		FString ProjectileSpawnSocketName = "muzzle";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
		TArray<FName> ProjectileSpawnSocketNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		ETurretOperationMode CurrentMode = ETurretOperationMode::Rest;

	// sets the order in which the turret rotates towards the target rotation
	// - set this to Simultaneously when using Track-Operationmode
	// - increase FinalAdjustmentSpeed when using a non-simultaneous order to increase visual feedback of the separation of the rotations
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		ETurretRotationOrder RotationOrder = ETurretRotationOrder::Simultaneously;

	// Rotationspeed in [deg/s]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.01", ClampMax = "360000.0", UIMin = "0.01", UIMax = "360.0"))
		float TurretYawRotationSpeed = 45.0f;

	// Rotationspeed in [deg/s]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.01", ClampMax = "360000.0", UIMin = "0.01", UIMax = "360.0"))
		float TurretPitchRotationSpeed = 45.0f;

	// enable switching from a constant rotation to a slower and smoother rotation for final aim adjustment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
		bool bUseSmoothFinalAdjustment = true;

	// InterpSpeed for smooth rotating before reaching target rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.01", ClampMax = "360000.0", UIMin = "0.01", UIMax = "360.0"))
		float FinalAdjustmentSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Limits", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "0.0"))
		float TurretYawLimitLeft = -180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Limits", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "0.0", UIMax = "180.0"))
		float TurretYawLimitRight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Limits", meta = (ClampMin = "-90.0", ClampMax = "90.0", UIMin = "0.0", UIMax = "90.0"))
		float TurretPitchLimitUp = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Limits", meta = (ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "0.0"))
		float TurretPitchLimitDown = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
		FVector TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
		FRotator TargetRelativeAimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
		FRotator CurrentRelativeAimRotation;

	// returns the current yaw rotation speed mapped to 0.0-1.0; 1.0 indicates speed is at max
	UFUNCTION(BlueprintCallable, Category = "Turret|Rotation")
		float GetCurrentYawRotationSpeed()
	{
		return FMath::Abs(CurrentYawRotationSpeed / TurretYawRotationSpeed);
	}

	// returns the current picth rotation speed mapped to 0.0-1.0; 1.0 indicates speed is at max
	UFUNCTION(BlueprintCallable, Category = "Turret|Rotation")
		float GetCurrentPitchRotationSpeed()
	{
		return FMath::Abs(CurrentPitchRotationSpeed / TurretPitchRotationSpeed);
	}

private:
	FRotator ResultYaw;
	FRotator ResultPitch;
	// enables yaw rotation
	bool bCanYaw;
	// enables pitch rotation
	bool bCanPitch;
	FVector PreviousLocation;
	FVector Velocity;
	float CurrentPitchRotationSpeed;
	float CurrentYawRotationSpeed;

};
