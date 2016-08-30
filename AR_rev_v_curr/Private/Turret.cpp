// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Turret.h"

// Sets default values
ATurret::ATurret(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Root = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("Root"));
	RootComponent = Root;
	TurretYawPart = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("TurretYawPart"));
	TurretPitchPart = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("TurretPitchPart"));

	TurretYawPart->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform, RootConnectionSocket);
	TurretPitchPart->AttachToComponent(TurretYawPart, FAttachmentTransformRules::KeepRelativeTransform, YawPartConnectionSocket);
	TurretPitchPart->AttachToComponent(TurretYawPart, FAttachmentTransformRules::KeepRelativeTransform, YawPartConnectionSocket);

	PreviousLocation = GetActorLocation();
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

	CalcActorVelocityVector(DeltaTime);

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, Velocity.ToString());

	switch (CurrentMode)
	{
	case ETurretOperationMode::Rest: {
		SetRestingAimLocation();
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation();
		RotateTurret();
		if (CheckAimFinished())
		{
			SetActorTickEnabled(false);
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

void ATurret::CalcActorVelocityVector(const float& DeltaTime)
{
	if (DeltaTime != 0.0f) {
		Velocity = (GetActorLocation() - PreviousLocation) / DeltaTime;
	}
	else
	{
		Velocity = FVector::ZeroVector;
	}
	PreviousLocation = GetActorLocation();
}

void ATurret::UpdateTurret(const FVector& TargetedLocation, const ETurretOperationMode& NewOperationMode)
{
	TargetLocation = TargetedLocation;
	CurrentMode = NewOperationMode;
	SetActorTickEnabled(NewOperationMode != ETurretOperationMode::Freeze);
}

bool ATurret::Fire(const bool bAcceptInaccuracy, const float Accuracy) const
{
	if (bAcceptInaccuracy)
	{
		// Fire instantly
		return true;
	}
	if (CheckAimFinished(Accuracy) && CurrentMode != ETurretOperationMode::Rest)
	{
		// Fire
		return true;
	}
	return false;
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
		FRotator Rotation = DirVec.Rotation();
		FTransform RelativeAimTransform = FTransform(Rotation).GetRelativeTransform(GetActorTransform());
		TargetRelativeAimRotation = RelativeAimTransform.Rotator();
	}
}

void ATurret::CalculateCurrentRelativeAimRotation()
{
	if (TurretPitchPart) {
		CurrentRelativeAimRotation = TurretPitchPart->GetComponentTransform().GetRelativeTransform(GetTransform()).Rotator();
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
	if (TurretYawPart && GetWorld())
	{
		const FVector& RotationAxis = TurretYawPart->GetRelativeTransform().GetRotation().GetUpVector();
		const FRotator Delta = (CurrentRelativeAimRotation - TargetRelativeAimRotation).GetNormalized();
		const float YawRotationDirection = -FMath::Sign(Delta.Yaw);

		const FVector NewDirection = CurrentRelativeAimRotation.Vector().RotateAngleAxis(TurretYawRotationSpeed * GetWorld()->DeltaTimeSeconds, YawRotationDirection * RotationAxis);
		ResultYaw = NewDirection.GetUnsafeNormal().Rotation();
	}
}

void ATurret::CalculateConstantPitchRotation()
{
	if (TurretPitchPart && GetWorld())
	{
		const FVector& RotationAxis = CurrentRelativeAimRotation.Quaternion().GetRightVector();
		const FRotator Delta = (CurrentRelativeAimRotation - TargetRelativeAimRotation).GetNormalized();
		const float PitchRotationDirection = FMath::Sign(Delta.Pitch);

		const FVector NewDirection = CurrentRelativeAimRotation.Vector().RotateAngleAxis(TurretPitchRotationSpeed * GetWorld()->DeltaTimeSeconds, PitchRotationDirection * RotationAxis);
		ResultPitch = NewDirection.GetSafeNormal().Rotation();
	}
}

void ATurret::ChooseRotations()
{
	if (GetWorld()) {
		const FRotator SmoothedNewRotation = FMath::RInterpTo(CurrentRelativeAimRotation, TargetRelativeAimRotation, GetWorld()->DeltaTimeSeconds, FinalAdjustmentSpeed);

		const FRotator Delta_Smooth_Current = (SmoothedNewRotation - CurrentRelativeAimRotation).GetNormalized();
		const FRotator Delta_ResultPitch_Current = (ResultPitch - CurrentRelativeAimRotation).GetNormalized();
		const FRotator Delta_ResultYaw_Current = (ResultYaw - CurrentRelativeAimRotation).GetNormalized();

		if (FMath::Abs(Delta_Smooth_Current.Pitch) <= FMath::Abs(Delta_ResultPitch_Current.Pitch)) {
			ResultPitch = SmoothedNewRotation;
			if (RotationOrder == ETurretRotationOrder::PitchYaw)
			{
				bCanPitch = true;
				bCanYaw = true;
			}
		}
		else
		{
			if (RotationOrder == ETurretRotationOrder::PitchYaw)
			{
				bCanPitch = true;
				bCanYaw = false;
			}
		}
		if (FMath::Abs(Delta_Smooth_Current.Yaw) <= FMath::Abs(Delta_ResultYaw_Current.Yaw)) {
			ResultYaw = SmoothedNewRotation;
			if (RotationOrder == ETurretRotationOrder::YawPitch)
			{
				bCanYaw = true;
				bCanPitch = true;
			}
		}
		else
		{
			if (RotationOrder == ETurretRotationOrder::YawPitch)
			{
				bCanYaw = true;
				bCanPitch = false;
			}
		}
	}
}

void ATurret::RotateTurret()
{
	if (RotationOrder == ETurretRotationOrder::Simultaneously)
	{
		bCanYaw = true;
		bCanPitch = true;
	}
	if (TurretYawPart && bCanYaw)
	{
		TurretYawPart->SetRelativeRotation(FRotator(0.0f, ResultYaw.Yaw, 0.0f));
	}
	if (TurretPitchPart && bCanPitch)
	{
		TurretPitchPart->SetRelativeRotation(FRotator(ResultPitch.Pitch, 0.0f, 0.0f));
	}
}

bool ATurret::CheckAimFinished(const float& Precision) const
{
	const FRotator Delta = (TargetRelativeAimRotation - CurrentRelativeAimRotation).GetNormalized();
	const float PrecSquared = Precision*Precision;
	return Delta.Pitch * Delta.Pitch < PrecSquared && Delta.Yaw * Delta.Yaw < PrecSquared;
} 
