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
	GetProjectileSpawnPointSockets();
}

void ATurret::GetProjectileSpawnPointSockets()
{
	ProjectileSpawnSocketNames.Empty();
	if (!TurretPitchPart) return;
	for (FName Name : TurretPitchPart->GetAllSocketNames())
	{
		if (Name.ToString().StartsWith(ProjectileSpawnSocketName))
		{
			ProjectileSpawnSocketNames.Add(Name);
		}
	}
}

// Called every frame
void ATurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CalcTurretWorldVelocityVector(DeltaTime);
	
	switch (CurrentMode)
	{
	case ETurretOperationMode::Rest: {
		SetRestingAimLocation();
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation(DeltaTime);
		RotateTurret(DeltaTime);
		if (CheckAimFinished())
		{
			SetActorTickEnabled(false);
		}
	}
									 break;
	case ETurretOperationMode::Track: {
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation(DeltaTime);
		RotateTurret(DeltaTime);
	}
									  break;
	case ETurretOperationMode::AimOnce: {
		GetAimDirection(TargetLocation);
		CalculateCurrentRelativeAimRotation();
		CalculateAndChooseRotation(DeltaTime);
		RotateTurret(DeltaTime);
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

void ATurret::CalcTurretWorldVelocityVector(const float& DeltaTime)
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
	// TODO: potential bug (turret not moving)?
	//if (TargetLocation.Equals(TargetedLocation, 1.0f)) return;
	TargetLocation = TargetedLocation;
	CurrentMode = NewOperationMode;
	SetActorTickEnabled(NewOperationMode != ETurretOperationMode::Freeze);
}

bool ATurret::Fire(const bool bAcceptInaccuracy, const float Accuracy)
{
	if (bAcceptInaccuracy)
	{
		// Fire instantly
		StartFiring();
		return true;
	}
	if (CheckAimFinished(Accuracy) && CurrentMode != ETurretOperationMode::Rest)
	{
		// Fire
		StartFiring();
		return true;
	}
	return false;
}

void ATurret::StartFiring_Implementation()
{
}

void ATurret::SetRestingAimLocation()
{
	if (TurretYawPart && TurretYawPart->DoesSocketExist(YawPartConnectionSocket)) {
		// TODO: make resting direction editable
		TargetLocation = TurretYawPart->GetSocketLocation(YawPartConnectionSocket) + GetActorForwardVector();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, TargetLocation.ToString());
	}
	else
	{
		TargetLocation = GetActorLocation() + GetActorForwardVector();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Resting: Else-Case: " + TargetLocation.ToString());
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TargetLocation.ToString());
}

void ATurret::GetAimDirection(const FVector& TargetLoc)
{
	if (TurretPitchPart) {
		const FVector DirVec = (TargetLoc - TurretPitchPart->GetComponentLocation()).GetSafeNormal();
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

void ATurret::CalculateAndChooseRotation(const float& DeltaTime)
{
	CalculateConstantYawRotation(DeltaTime);
	CalculateConstantPitchRotation(DeltaTime);
	ChooseRotations(DeltaTime);
}

void ATurret::CalculateConstantYawRotation(const float& DeltaTime)
{
	if (TurretYawPart)
	{
		const FVector& RotationAxis = TurretYawPart->GetRelativeTransform().GetRotation().GetUpVector();
		const FRotator Delta = (CurrentRelativeAimRotation - TargetRelativeAimRotation).GetNormalized();
		const float YawRotationDirection = -FMath::Sign(Delta.Yaw);

		const FVector NewDirection = CurrentRelativeAimRotation.Vector().RotateAngleAxis(TurretYawRotationSpeed * DeltaTime, YawRotationDirection * RotationAxis);
		ResultYaw = NewDirection.GetUnsafeNormal().Rotation();
	}
}

void ATurret::CalculateConstantPitchRotation(const float& DeltaTime)
{
	if (TurretPitchPart)
	{
		const FVector& RotationAxis = CurrentRelativeAimRotation.Quaternion().GetRightVector();
		const FRotator Delta = (CurrentRelativeAimRotation - TargetRelativeAimRotation).GetNormalized();
		const float PitchRotationDirection = FMath::Sign(Delta.Pitch);

		const FVector NewDirection = CurrentRelativeAimRotation.Vector().RotateAngleAxis(TurretPitchRotationSpeed * DeltaTime, PitchRotationDirection * RotationAxis);
		ResultPitch = NewDirection.GetSafeNormal().Rotation();
	}
}

void ATurret::ChooseRotations(const float& DeltaTime)
{
	const FRotator SmoothedNewRotation = CalcFinalRotation(DeltaTime);

	const FRotator Delta_Smooth_Current = (SmoothedNewRotation - CurrentRelativeAimRotation).GetNormalized();
	const FRotator Delta_ResultPitch_Current = (ResultPitch - CurrentRelativeAimRotation).GetNormalized();
	const FRotator Delta_ResultYaw_Current = (ResultYaw - CurrentRelativeAimRotation).GetNormalized();

	if (FMath::Abs(Delta_Smooth_Current.Pitch) <= FMath::Abs(Delta_ResultPitch_Current.Pitch)) {
		CurrentPitchRotationSpeed = Delta_Smooth_Current.Pitch / DeltaTime;
		ResultPitch = SmoothedNewRotation;
		if (RotationOrder == ETurretRotationOrder::PitchYaw)
		{
			bCanPitch = true;
			bCanYaw = true;
		}
	}
	else
	{
		CurrentPitchRotationSpeed = Delta_ResultPitch_Current.Pitch / DeltaTime;
		if (RotationOrder == ETurretRotationOrder::PitchYaw)
		{
			bCanPitch = true;
			bCanYaw = false;
		}
	}
	if (FMath::Abs(Delta_Smooth_Current.Yaw) <= FMath::Abs(Delta_ResultYaw_Current.Yaw)) {
		CurrentYawRotationSpeed = Delta_Smooth_Current.Yaw / DeltaTime;
		ResultYaw = SmoothedNewRotation;
		if (RotationOrder == ETurretRotationOrder::YawPitch)
		{
			bCanYaw = true;
			bCanPitch = true;
		}
	}
	else
	{
		CurrentYawRotationSpeed = Delta_ResultYaw_Current.Yaw / DeltaTime;
		if (RotationOrder == ETurretRotationOrder::YawPitch)
		{
			bCanYaw = true;
			bCanPitch = false;
		}
	}
	if (RotationOrder == ETurretRotationOrder::Simultaneously)
	{
		bCanYaw = true;
		bCanPitch = true;
	}
}

FRotator ATurret::CalcFinalRotation(const float& DeltaTime)
{
	if (bUseSmoothFinalAdjustment) {
		return FMath::RInterpTo(CurrentRelativeAimRotation, TargetRelativeAimRotation, DeltaTime, FinalAdjustmentSpeed);
	}
	return TargetRelativeAimRotation;
}

void ATurret::RotateTurret(const float DeltaTime)
{
	const float Yaw = FMath::Clamp(ResultYaw.Yaw, TurretYawLimitLeft, TurretYawLimitRight);
	if (TurretYawPart && bCanYaw)
	{
		TurretYawPart->SetRelativeRotation(FRotator(0.0f, Yaw, 0.0f));
	}
	// Set Rotation Speed to zero when Limits are reached
	if (!bCanYaw || Yaw == TurretYawLimitLeft || Yaw == TurretYawLimitRight)
	{
		CurrentYawRotationSpeed = 0.0f;
	}

	const float Pitch = FMath::Clamp(ResultPitch.Pitch, TurretPitchLimitDown, TurretPitchLimitUp);
	if (TurretPitchPart && bCanPitch)
	{
		TurretPitchPart->SetRelativeRotation(FRotator(Pitch, 0.0f, 0.0f));
	}
	// Set Rotation Speed to zero when Limits are reached
	if (!bCanPitch || Pitch == TurretPitchLimitDown || Pitch == TurretPitchLimitUp)
	{
		CurrentPitchRotationSpeed = 0.0f;
	}

	// Audio Smoothing
	if (bUseAudioSmooting)
	{
		const float Alpha = FMath::Min(DeltaTime * AudioSmoothSpeed, 1.0f);
		CurrentYawRotationSpeed = FMath::Lerp(PrevYawRotationSpeed, CurrentYawRotationSpeed, Alpha);
		PrevYawRotationSpeed = CurrentYawRotationSpeed;
		CurrentPitchRotationSpeed = FMath::Lerp(PrevPitchRotationSpeed, CurrentPitchRotationSpeed, Alpha);
		PrevPitchRotationSpeed = CurrentPitchRotationSpeed;
	}
}

bool ATurret::CheckAimFinished(const float& Precision) const
{
	const FRotator Delta = (TargetRelativeAimRotation - CurrentRelativeAimRotation).GetNormalized();
	const float PrecSquared = Precision*Precision;
	return Delta.Pitch * Delta.Pitch < PrecSquared && Delta.Yaw * Delta.Yaw < PrecSquared;
}

float ATurret::GetCurrentYawRotationSpeed() const
{
	return FMath::Abs(CurrentYawRotationSpeed / TurretYawRotationSpeed);
}

float ATurret::GetCurrentPitchRotationSpeed() const
{
	return FMath::Abs(CurrentPitchRotationSpeed / TurretPitchRotationSpeed);
}
