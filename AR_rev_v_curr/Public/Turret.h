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

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	/* Set the AimLocation to be default Forward */
	void SetRestingAimLocation();

	/* Calculate the target rotation to be able to hit the location/target */
	void GetAimDirection(const FVector & TargetLocation);

	/* Calculate The Relative Rotation of the Pitch-Part (Barrels relative to Root) */
	void CalculateCurrentRelativeAimRotation();

	// Helper function that combines 3 function calls
	inline void CalculateAndChooseRotation();

	void CalculateConstantYawRotation();
	void CalculateConstantPitchRotation();
	void ChooseRotations();

	/* Rotates the turret using the choosen Rotation */
	void RotateTurret();

	/* Checks whether the Turret is aiming at the Target */
	bool CheckAimFinished(const float& Precision = 0.001f);

	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* Root;
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* TurretYawPart;
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UStaticMeshComponent* TurretPitchPart;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
	FName RootConnectionSocket = "None";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Construction")
	FName YawPartConnectionSocket = "pitch";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	ETurretOperationMode CurrentMode = ETurretOperationMode::Rest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
	FVector TargetLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
	FRotator TargetRelativeAimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Target")
		FRotator CurrentRelativeAimRotation;

};
