// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Turret.h"

// Sets default values
ATurret::ATurret(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.

	Root = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("Root"));
	RootComponent = Root;
	TurretYawPart = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("TurretYawPart"));
	TurretPitchPart = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("TurretPitchPart"));

	TurretYawPart->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform, RootConnectionSocket);
	TurretPitchPart->AttachToComponent(TurretYawPart, FAttachmentTransformRules::KeepRelativeTransform, YawPartConnectionSocket);


}

//allows calculation of missing values that have dependencies
void ATurret::PostInitProperties()
{
	Super::PostInitProperties();
	// do stuff e.g.
	// AdvancedMissileMinRange = MaxVelocity;
}

#if WITH_EDITOR
void ATurret::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// do stuff e.g.
	//AdvancedMissileMinRange = MaxVelocity;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Called when the game starts or when spawned
void ATurret::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (CurrentMode)
	{
	case ETurretOperationMode::Rest: {
		SetRestingAimLocation();
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation();
		RotateTurret();
		if(CheckAimFinished())
		{
			CurrentMode = ETurretOperationMode::Freeze;
		}
	}
									 break;
	case ETurretOperationMode::Track: {
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation();
		RotateTurret();
	}
									  break;
	case ETurretOperationMode::AimOnce: {
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation();
		RotateTurret();
		if (CheckAimFinished())
		{
			CurrentMode = ETurretOperationMode::Freeze;
		}
	}
										break;
	case ETurretOperationMode::Freeze: {
		SetActorTickEnabled(false);
	}
									   break;
	default: {
	}
	}
}

void ATurret::SetRestingAimLocation()
{
	if (TurretPitchPart) {
		// TODO: make resting direction editable
		TargetLocation = GetActorForwardVector() + TurretPitchPart->GetComponentLocation();
	}
}

void ATurret::GetAimDirection(const FVector& TargetLocation)
{
	if (TurretPitchPart) {
		const FVector DirVec = (TargetLocation - TurretPitchPart->GetComponentLocation()).GetSafeNormal();
		FQuat Rotation = DirVec.ToOrientationQuat();
		const FQuat RotationError = GetActorQuat() * Rotation.Inverse();
		TargetRelativeAimRotation = RotationError.Rotator();
		

	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TargetRelativeAimRotation.ToString());
}

void ATurret::CalculateCurrentRelativeAimRotation()
{
	if (TurretPitchPart) {
		CurrentRelativeAimRotation = (TurretPitchPart->GetComponentQuat() * GetActorQuat().Inverse()).Rotator();
	}
}

void ATurret::CalculateAndChooseRotation()
{
	CalculateConstantYawRotation();
	CalculateConstantPitchRotation();
	ChooseRotations();
}

void ATurret::CalculateConstantYawRotation()
{
}

void ATurret::CalculateConstantPitchRotation()
{
}

void ATurret::ChooseRotations()
{
}

void ATurret::RotateTurret()
{
	if(TurretYawPart)
	{
		float ResultYaw = TargetRelativeAimRotation.Yaw;
		TurretYawPart->SetRelativeRotation(FRotator(0.0f, 0.0f, ResultYaw));
	}
	if (TurretPitchPart)
	{
		float ResultPitch = TargetRelativeAimRotation.Pitch;
		TurretPitchPart->SetRelativeRotation(FRotator(0.0f, ResultPitch, 0.0f));
	}

}

bool ATurret::CheckAimFinished(const float& Precision)
{
	return false;
}
