// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "MainPawn.h"

#define DEBUG_MSG 0

bool AMainPawn::GetIsTargetable_Implementation()
{
	// TODO: when dead -> not target-able
	// if the timer for an active evasive action is active, the pawn is not target-able
	return !GetWorldTimerManager().IsTimerActive(EvasiveActionHandle);
}

// Sets default values
AMainPawn::AMainPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = false;
	bReplicateMovement = false;

	//SetActorEnableCollision(true);

	//Create components
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this,TEXT("RootComponent"));

	// the aircraft frame
	ArmorMesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("ArmorMesh"));
	ArmorMesh->OnComponentHit.AddDynamic(this, &AMainPawn::ArmorHit);
	RootComponent = ArmorMesh;

	ArmorMesh->SetCollisionObjectType(ECC_Pawn);
	ArmorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArmorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ArmorMesh->SetLinearDamping(0.0f);
	ArmorMesh->SetSimulatePhysics(true);
	ArmorMesh->SetEnableGravity(false);

	// the springarm for the camera
	SpringArm = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(this, TEXT("CameraSpringArm"));
	SpringArm->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
	//SpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 50.0f), FRotator(0.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 0.0f;
	SpringArmLength = SpringArm->TargetArmLength;
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 100.0f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.0f;
	SpringArm->CameraLagMaxDistance = 2500.0f;

	// the camera
	Camera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(this, TEXT("GameCamera"));
	Camera->AttachToComponent(SpringArm, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);

	// PostProcessSettings
	Camera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	Camera->PostProcessSettings.LensFlareIntensity = 0.0f;
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = 0.5f;
	Camera->PostProcessSettings.bOverride_AntiAliasingMethod = true;
	Camera->PostProcessSettings.AntiAliasingMethod = EAntiAliasingMethod::AAM_TemporalAA;
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.2f;

	// default gun sockets
	GunSockets.Add("gun0");
	GunSockets.Add("gun1");
	// default missile sockets
	MissileSockets.Add("missile0");
	MissileSockets.Add("missile1");

	//Take control of the default Player
	//AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AMainPawn::ArmorHit(UPrimitiveComponent* ThisComponent, class AActor* OtherActor, class UPrimitiveComponent* OtherComponent, FVector Loc, const FHitResult& FHitResult)
{
	// TODO: prevent collisionhandling when colliding with
	// - destructable
	// - missiles / projectiles
	// TODO: start anticollision system after a delay

	// enable the recover-from-collision system
	CollisionHandling = true;
	// location and normal for plane generation
	MostRecentCrashPoint = FHitResult.Location;
	CrashNormal = FHitResult.Normal;
}

void AMainPawn::RecoverFromCollision(const float DeltaSeconds)
{
	// system is activ and player has not stopped
	if (CollisionHandling && bCanReceivePlayerInput)
	{
		const float SafeDistance = 3000.0f;
		const float AntiCollisionSystemStrength = 0.01f;

		// orthogonal distance to plane where collision happened
		const float CurrDistanceToColl = FVector::PointPlaneDist(GetActorLocation(), MostRecentCrashPoint, CrashNormal);
		// distance from safety distance (20m)
		const float DeltaDistance = SafeDistance - CurrDistanceToColl;
		// factor to slow down the vehicle to prevent overshooting the point of safedistance
		const float Derivative = (DeltaDistance - PrevSafetyDistanceDelta) / PrevDeltaTime;
		// Velocity that will be added to the vehicle in order to get away from the collision point
		AntiCollisionVelocity = (DeltaDistance + 0.5f * Derivative) * (CollisionTimeDelta * AntiCollisionSystemStrength);
		// apply the the impuls away from the collision point
		ArmorMesh->AddImpulse(CrashNormal * AntiCollisionVelocity, NAME_None, true);
		// add the elapsed time
		CollisionTimeDelta += DeltaSeconds;
		// reset the collision handling system when either:
		// - the timelimit has been passed
		// - distance to safe distance has been passed
		// - the vehicle got behind the original collisionpoint
		if (CollisionTimeDelta > TimeOfAntiCollisionSystem || DeltaDistance < 0.0f || CurrDistanceToColl > SafeDistance)
		{
			CollisionHandling = false;
			AntiCollisionVelocity = 0.0f;
			CollisionTimeDelta = 0.0f;
			PrevSafetyDistanceDelta = 0.0f;
		}
		// store data for next tick to calculate the Derivative for slowing down
		PrevDeltaTime = DeltaSeconds;
		PrevSafetyDistanceDelta = DeltaDistance;
	}
}


void AMainPawn::InitWeapon()
{
	WeaponSpreadRadian = WeaponSpreadHalfAngle * PI / 180.0f;
	GunSalveIntervall = (GunSalveDensity * FireRateGun) / GunNumSalves;
	bGunHasAmmo = GunAmmunitionAmount > 0;

	MissileSpreadRadian = MissileSpreadHalfAngle * PI / 180.0f;
	MissileSalveIntervall = (MissileSalveDensity * FireRateMissile) / MissileNumSalves;
	bMissileHasAmmo = MissileAmmunitionAmount > 0;
}

void AMainPawn::InitRadar()
{
	MultiTargetLockOnAngleRad = FMath::Cos(MultiTargetLockOnAngleDeg / 180.0f * PI);
	MissileLockOnAngleRad = FMath::Cos(MissileLockOnAngleDeg / 180.0f * PI);
	GunLockOnAngleRad = FMath::Cos(GunLockOnAngleDeg / 180.0f * PI);

#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, " MultiTarget LockOn in radian : " + FString::SanitizeFloat(MultiTargetLockOnAngleRad));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, " Missile LockOn in radian : " + FString::SanitizeFloat(MissileLockOnAngleRad));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, " Gun LockOn in radian : " + FString::SanitizeFloat(GunLockOnAngleRad));
#endif

	RadarMaxLockOnRangeSquarred = RadarMaxLockOnRange * RadarMaxLockOnRange;
}

void AMainPawn::InitNetwork()
{
	NumberOfBufferedNetUpdates = FMath::RoundToInt(Smoothing * NetUpdateFrequency);
	NumberOfBufferedNetUpdates = FMath::Max(NumberOfBufferedNetUpdates, 2);
	LerpVelocity = NetUpdateFrequency / NumberOfBufferedNetUpdates;
	PredictionAmount = (NumberOfBufferedNetUpdates + AdditionalUpdatePredictions) / NetUpdateFrequency;
}

// Called when the game starts or when spawned
void AMainPawn::BeginPlay()
{
	Super::BeginPlay();

	// start with player stopped
	bCanReceivePlayerInput = false;

	InitWeapon();
	InitRadar();
	InitVelocities();
	InitNetwork();

	// initialize variables in order to prevent crashes
	lastUpdate = GetWorld()->RealTimeSeconds;

	// make sure the vehicle mass is the same on all instances
	ArmorMesh->SetMassOverrideInKg(NAME_None, 15000.0f, true);
}

// Called every frame
void AMainPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentVelocitySize = GetVelocity().Size();

	// handle player input	
	if (IsLocallyControlled())
	{
		// choose mouseinput method
		if (bUseInternMouseSensitivity)
		{
			InputAxis += CameraInput * MouseSensitivity;
		}
		else
		{
			// get mouse position
			GetCursorLocation(CursorLoc);

			// get viewport size/center
			GetViewportSizeCenter(ViewPortSize, ViewPortCenter);
			InputAxis = (CursorLoc - ViewPortCenter) / ViewPortCenter;
		}
		// clamp the axis to make sure the player can't turn more than maxTurnrate
		InputAxis.X = FMath::Clamp(InputAxis.X, -1.0f, 1.0f);
		InputAxis.Y = FMath::Clamp(InputAxis.Y, -1.0f, 1.0f);

		float InputAxisLength = MouseInput.Size();
		if (InputAxisLength > 1.0f)
		{
			InputAxis.X /= InputAxisLength;
			InputAxis.Y /= InputAxisLength;
		}
		MouseInput = InputAxis;
		//MouseInput = InputAxis * InputAxis.GetSafeNormal().GetAbsMax();

#if DEBUG_MSG == 1
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat(InputAxis.X) + " x " + FString::SanitizeFloat(InputAxis.Y));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat((InputAxis * InputAxis.GetSafeNormal().GetAbsMax()).Size()));


		// debugging:
		FPlayerInputPackage test;
		test.setPacketNumber(123);
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::FromInt(test.getPacketNumber()));
		test.IncrementPacketNumber();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::FromInt(test.getPacketNumber()));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::FromInt(test.getPacketNumber()));
#endif

		// deadzone with no turning -> mouseinput interpreted as zero
		if (MouseInput.SizeSquared() < Deadzone * Deadzone)
		{
			MouseInput = FVector2D::ZeroVector;
		}
		else
		{
			// converting the mouseinput to use more precise turning around the screencenter
			MouseInput.X = (1.0f - FMath::Cos(InputAxis.X * HalfPI)) * FMath::Sign(InputAxis.X);
			MouseInput.Y = (1.0f - FMath::Cos(InputAxis.Y * HalfPI)) * FMath::Sign(InputAxis.Y);

			// lerping with the customizable Precisionfactor
			MouseInput.X = FMath::Lerp(InputAxis.X, MouseInput.X, CenterPrecision);
			MouseInput.Y = FMath::Lerp(InputAxis.Y, MouseInput.Y, CenterPrecision);


			float MouseInputLength = MouseInput.Size();
			if (MouseInputLength > 1.0f)
			{
				MouseInput.X /= MouseInputLength;
				MouseInput.Y /= MouseInputLength;
			}


			//MouseInput *= MouseInput.GetSafeNormal().GetAbsMax();
		}

		if (bFreeCameraActive)
		{
			// rotation from mousemovement (input axis lookup and lookright)
			CurrentSpringArmRotation = CurrentSpringArmRotation * FQuat(FRotator(CameraInput.Y * -FreeCameraSpeed, CameraInput.X * FreeCameraSpeed, 0.0f));

			// rotate the camera with the springarm
			SpringArm->SetWorldRotation(CurrentSpringArmRotation);

			// disable rotationcontrol
			MouseInput = FVector2D::ZeroVector;

			// disable strafeinput
			MovementInput.Y = 0.0f;
		}

		RawTurnInput = MouseInput;

		// disable input when player has stopped
		if (!bCanReceivePlayerInput)
		{
			MovementInput = MouseInput = FVector2D::ZeroVector;
		}

		if (Role < ROLE_Authority)
		{
			// keep track of how many packages have been created		
			InputPackage.IncrementPacketNumber();
			// store the player input
			InputPackage.setMouseInput(MouseInput);
			//InputPackage.MouseInput = MouseInput;
			InputPackage.SetMovementInput(MovementInput);

#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(MouseInput.X) + " x " + FString::SanitizeFloat(MouseInput.Y));
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(MouseInput.Size()));
#endif

			// store client movement for later correction when correct server transform is received
			FPositionHistoryElement CurrentPositionData;
			CurrentPositionData.PacketNo = InputPackage.getPacketNumber();
			CurrentPositionData.Transform = GetTransform();
			MovementHistory.Add(CurrentPositionData);

			// send the input to server
#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, "Sending Packet = " + FString::FromInt(InputPackage.getPacketNumber()));
#endif
			GetPlayerInput(InputPackage);
		}

		// radar and weapons only on player side
		// TODO: cheating possible?
		WeaponLock();
	}

	if (Role == ROLE_Authority)
	{
		TargetLock();

		// unpack received inputdata package
		if (!IsLocallyControlled())
		{
			MouseInput = InputPackage.getMouseInput();
			MovementInput = InputPackage.GetMovementInput();
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, "Server receiving Movement :" + FString::SanitizeFloat(MovementInput.Size()));
		}
		if (!bCanReceivePlayerInput)
		{
			MovementInput = MouseInput = FVector2D::ZeroVector;
		}


		// player movement on authority
		MainPlayerMovement(DeltaTime);

		// movement replication

		TransformOnAuthority = ArmorMesh->GetComponentTransform();
		LinearVelocity = ArmorMesh->GetPhysicsLinearVelocity();
		//AngularVelocity = ArmorMesh->GetPhysicsAngularVelocity();
	}

	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Locally Controlled Client");

		// clientside movement (predicting; will be corrected with information from authority)
		MainPlayerMovement(DeltaTime, LinVelError, AngVelError);
	}

	if (Role < ROLE_Authority && !IsLocallyControlled())
	{
		//if (ArmorMesh->IsSimulatingPhysics()) {
		ArmorMesh->SetSimulatePhysics(true);
		ArmorMesh->SetEnableGravity(false);
		ArmorMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		ArmorMesh->SetPhysicsAngularVelocity(FVector::ZeroVector);


		//}
		GetPing();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "Ping = " + FString::SanitizeFloat(Ping) + " s");
		// proxyclient movement
		LerpProgress += DeltaTime;
		float convertedLerpFactor = FMath::Clamp(LerpProgress * LerpVelocity, 0.0f, 1.0f);
		FTransform NewTransform;
		NewTransform.Blend(TransformOnClient, TargetTransform, convertedLerpFactor);
		// transform actor to new location/rotation
		SetActorTransform(NewTransform, false, nullptr, ETeleportType::None);
	}
}

// replication of variables
void AMainPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AMainPawn, bGunHasAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, bGunReady, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, bMissileHasAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, bMissileReady, COND_OwnerOnly);
	DOREPLIFETIME(AMainPawn, TransformOnAuthority);
	DOREPLIFETIME_CONDITION(AMainPawn, LinearVelocity, COND_SkipOwner);
	//DOREPLIFETIME(AMainPawn, AngularVelocity);
	//DOREPLIFETIME(AMainPawn, WorldAngVel);
	//DOREPLIFETIME(AMainPawn, TargetLinearVelocity);
	DOREPLIFETIME_CONDITION(AMainPawn, bCanReceivePlayerInput, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, bMultiTarget, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, AutoLevelAxis, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AMainPawn, AuthorityAck, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(AMainPawn, bGunFire, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AMainPawn, bMissileFire, COND_SkipOwner);
	DOREPLIFETIME(AMainPawn, MainLockOnTarget);
	DOREPLIFETIME(AMainPawn, MultiTargets);
	DOREPLIFETIME(AMainPawn, bHasGunLock);
}

void AMainPawn::OnRep_MainLockOnTarget()
{
	TargetListChanged();
}

void AMainPawn::OnRep_MultiTargets()
{
	TargetListChanged();
}

void AMainPawn::TargetListChanged_Implementation()
{
};

void AMainPawn::MainPlayerMovement(const float DeltaTime, const FVector& CorrectionVelocity, const FVector& CorrectionAngularVelocity)
{
	// equal for locally controlled --------------------------------------------------------------
	FVector2D PrevUsedMouseInput = PreviousMouseInput;
	{
		// yaw smoothing
		if (PrevUsedMouseInput.X * MouseInput.X > 0.0f
			&& MouseInput.X * MouseInput.X < PrevUsedMouseInput.X * PrevUsedMouseInput.X) // new x is smaller
		{
			MouseInput.X = FMath::FInterpTo(PrevUsedMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed * ResetSpeed);
		}
		else
		{
			MouseInput.X = FMath::FInterpTo(PrevUsedMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed);
		}

		// pitch smoothing
		if (PrevUsedMouseInput.Y * MouseInput.Y > 0.0f
			&& MouseInput.Y * MouseInput.Y < PrevUsedMouseInput.Y * PrevUsedMouseInput.Y) // new y is smaller
		{
			MouseInput.Y = FMath::FInterpTo(PrevUsedMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed * ResetSpeed);
		}
		else
		{
			MouseInput.Y = FMath::FInterpTo(PrevUsedMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed);
		}
	}

	// player is flying and not stopped
	if (bCanReceivePlayerInput)
	{
		// Forward Velocity during flight
		if (MovementInput.X > 0.0f)
		{
			// forward
			ForwardVel = FMath::FInterpTo(ForwardVel, MovementInput.X * VelForwardDelta + DefaultForwardVel, DeltaTime, ForwardAcceleration);
		}
		else
		{
			// backwards
			ForwardVel = FMath::FInterpTo(ForwardVel, MovementInput.X * VelBackwardsDelta + DefaultForwardVel, DeltaTime, BackwardsAcceleration);
		}
		// Strafe Velocity during flight
		if (bUseConstantStrafeAcceleration)
		{
			if (MovementInput.Y == 0.0f)
			{
				StrafeVel = FMath::FInterpTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, StrafeBankAcceleration);
			}
			else
			{
				StrafeVel = FMath::FInterpConstantTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, ConstantStrafeAcceleration);
			}
		}
		else
		{
			StrafeVel = FMath::FInterpTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, StrafeBankAcceleration);
		}
	}
	else
	{
		// player has not input/has stopped
		ForwardVel = FMath::FInterpTo(ForwardVel, 0.0f, DeltaTime, BackwardsAcceleration);
		StrafeVel = FMath::FInterpTo(StrafeVel, 0.0f, DeltaTime, StrafeBankAcceleration);
	}

	// after a collision disable playerinput for a specified amount of time
	if (MovControlStrength < TimeOfNoControl)
	{
		ForwardVel = StrafeVel = 0.0f;
	}

	// select new bankrotation either from strafe input or from current turnvalue 
	if (MovementInput.Y != 0)
	{
		// rot from strafeinput
		CurrStrafeRot = FMath::FInterpTo(PrevStrafeRot, MovementInput.Y * -MaxStrafeBankAngle, DeltaTime, StrafeBankAcceleration);
	}
	else
	{
		// rot from turning
		CurrStrafeRot = FMath::FInterpTo(PrevStrafeRot, MouseInput.X * -MaxStrafeBankAngle, DeltaTime, TurnInterpSpeed);
	}

	// deltarotation to previous tick
	const float DeltaRot = PrevStrafeRot - CurrStrafeRot;

	// store current bankrotation for next tick
	PrevStrafeRot = CurrStrafeRot;

	// rotation
	{
		// angular velocity from playerinput in actor local space
		const FVector LocalRotVel = FVector(0.0f, MouseInput.Y * MaxTurnRate, MouseInput.X * MaxTurnRate);
		// converted angular velocity in worldspace
		WorldAngVel = GetActorRotation().RotateVector(LocalRotVel);
		// compensate for bankrotation
		WorldAngVel = WorldAngVel.RotateAngleAxis(-CurrStrafeRot, GetActorForwardVector());

#if DEBUG_MSG == 1
		// print current absolut turnrate (angular velocity)
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::SanitizeFloat(WorldAngVel.Size()) + " deg/sec");
#endif

		// collisionhandling: is set to zero on "each" collision and recovers in 2 seconds
		RotControlStrength = FMath::FInterpConstantTo(RotControlStrength, TimeOfNoControl + 1.0f, DeltaTime, 1.0f);

		// if 1 second has passed and not yet fully recovered
		if (RotControlStrength > TimeOfNoControl && RotControlStrength < TimeOfNoControl + 1.0f)
		{
			ArmorMesh->SetAngularDamping(0.0f);
			const float ControlAlpha = FMath::Square(RotControlStrength - TimeOfNoControl);
#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Orange, "ROTATION control CHARGING " + FString::SanitizeFloat(ControlAlpha));
#endif
			const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime, 0.0f, 0.0f));

			// blend between pure physics velocities and player caused velocity				
			const FVector NewAngVel = FMath::Lerp(ArmorMesh->GetPhysicsAngularVelocity(), WorldAngVel - AngVelStrafeCompensation, ControlAlpha);
			ArmorMesh->SetPhysicsAngularVelocity(NewAngVel + CorrectionAngularVelocity);

			// rotate springarm/camera in local space to compensate for straferotation 
			if (!bFreeCameraActive)
			{
				SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
			}
		}
		// no collision handling (normal flight)
		else if (RotControlStrength >= TimeOfNoControl + 1.0f)
		{
#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, "ROTATION control FULL");
#endif
			// auto level function  
			if (LevelVel > 0.0f && !bFreeCameraActive)
			{
				// TODO: get rid of Acos
				// dot product between gravity vector and actor rightvector
				const float DotUpRight = FVector::DotProduct(Camera->GetRightVector(), bUseGravityDirForAutoLevel ? AutoLevelAxis : FVector(0.0f, 0.0f, 1.0f));
				const float LevelHorizonVel = LevelVel * (90.0f - FMath::Acos(DotUpRight) * 180.0f / PI);

				const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime + LevelHorizonVel, 0, 0));

				// player input is directly translated into movement
				ArmorMesh->SetPhysicsAngularVelocity(WorldAngVel - AngVelStrafeCompensation + CorrectionAngularVelocity);
				// rotate springarm/camera in local space to compensate for straferotation 
				SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot + LevelHorizonVel * DeltaTime), false, nullptr, ETeleportType::None);
			}
			else
			{
				const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime, 0, 0));

				// player input is directly translated into movement
				ArmorMesh->SetPhysicsAngularVelocity(WorldAngVel - AngVelStrafeCompensation + CorrectionAngularVelocity);
				// rotate springarm/camera in local space to compensate for straferotation 
				if (!bFreeCameraActive)
				{
					SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
				}
			}
		}
		else
		{
			// RotControlStrength is between 0 and TimeOfNoControl -> player has no control: collision or input disabled
			ArmorMesh->SetAngularDamping(5.0f);
#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "ROTATION control DEACTIVATED");
#endif
			if (!bFreeCameraActive)
			{
				SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
			}
		}
	}

	// location
	{
		TargetLinearVelocity = GetActorForwardVector() * ForwardVel + GetActorRightVector() * StrafeVel;

		// straferotation compensation 
		TargetLinearVelocity = TargetLinearVelocity.RotateAngleAxis(-CurrStrafeRot, GetActorForwardVector()) + CorrectionVelocity;

		MovControlStrength = FMath::FInterpConstantTo(MovControlStrength, TimeOfNoControl + 1.0f, DeltaTime, 1.0f);

		// return the control to the player over a time of 1 second
		if (MovControlStrength > TimeOfNoControl && MovControlStrength < TimeOfNoControl + 1.0f)
		{
			ArmorMesh->SetPhysicsLinearVelocity(FMath::Lerp(ArmorMesh->GetPhysicsLinearVelocity(), TargetLinearVelocity, MovControlStrength - TimeOfNoControl));
		}
		else if (MovControlStrength >= TimeOfNoControl + 1.0f)
		{
			// normal flight: set velocities directly
			ArmorMesh->SetPhysicsLinearVelocity(TargetLinearVelocity);
		}
		// move away from collision location
		RecoverFromCollision(DeltaTime);

		// preparation for next tick
		PreviousMouseInput = MouseInput;
	}

	// end both -----------------------------------------
}

void AMainPawn::OnRep_TransformOnAuthority()
{
	// calculate the time between this and the previous update
	if (GetWorld())
	{
		NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
		lastUpdate = GetWorld()->RealTimeSeconds;
	}

	// uncontrolled client
	if (Role < ROLE_Authority && !IsLocallyControlled())
	{
		//Alpha = GetWorld()->DeltaTimeSeconds;

		// store starttransform
		TransformOnClient = GetTransform();
		// reset blendfactor
		LerpProgress = 0.0f;

		const FVector Direction = LinearVelocity.GetSafeNormal();
		const float Velocity = LinearVelocity.Size() * PredictionAmount;

		TargetTransform = TransformOnAuthority;
		TargetTransform.AddToTranslation(Direction * Velocity);
		return;
	}

	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		// vector between server location and client location
		// velocity to correct the client
		LinVelError = (TransformOnAuthority.GetLocation() - PastClientTransform.GetLocation()) /*NetDelta*/;
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Orange, "         Location Error  = " + FString::SanitizeFloat(LinVelError.Size() * 0.01f) + " m");
#endif
		// the rotation delta between client and server
		const FQuat RotationError = TransformOnAuthority.GetRotation() * PastClientTransform.GetRotation().Inverse();
		const FRotator ErrorDelta = RotationError.Rotator();
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "Yaw   Error  " + FString::SanitizeFloat(ErrorDelta.Yaw));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "Pitch Error  " + FString::SanitizeFloat(ErrorDelta.Pitch));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "Roll  Error  " + FString::SanitizeFloat(ErrorDelta.Roll));
#endif
		// angular velocity to correct the client
		AngVelError = FVector(-ErrorDelta.Roll, -ErrorDelta.Pitch, ErrorDelta.Yaw)/* * NetDelta*/;

		// teleport the client if the distance to correct location is too big
		if (LinVelError.Size() > 5000.0f)
		{
			SetActorLocation(TransformOnAuthority.GetLocation(), false, nullptr, ETeleportType::None);
			LinVelError = FVector::ZeroVector;
		}

		// if rotation delta is too big add the correction
		if (AngVelError.Size() > 90.0f)
		{
			AddActorWorldRotation(RotationError, false, nullptr, ETeleportType::None);
			//SetActorRotation(TransformOnAuthority.GetRotation(), ETeleportType::None);
			AngVelError = FVector::ZeroVector;
		}
	}
}

//void AMainPawn::ApplyTranformCorrection()

void AMainPawn::OnRep_LinearVelocity()
{
	//	if (Role < ROLE_Authority) {
	//		if (ArmorMesh->IsSimulatingPhysics()) ArmorMesh->SetPhysicsLinearVelocity(LinearVelocity, false);
	//	}
}

void AMainPawn::OnRep_AngularVelocity()
{
	//if (Role < ROLE_Authority) {
	//	if (ArmorMesh->IsSimulatingPhysics()) ArmorMesh->SetPhysicsAngularVelocity(AngularVelocity, false);
	//}
}

void AMainPawn::InitVelocities()
{
	VelForwardDelta = MaxVelocity - DefaultForwardVel;
	VelBackwardsDelta = -MinVelocity + DefaultForwardVel;

	ConstantStrafeAcceleration = FMath::Abs(MaxStrafeVel / TimeToMaxStrafeVel);
}

void AMainPawn::GetPing()
{
	if (State)
	{
		Ping = State->ExactPing * 0.001f;
		return;
	}
	if (GetWorld()->GetFirstPlayerController())
	{ // get ping
		State = Cast<APlayerState>(
			GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
		if (State)
		{
			Ping = State->ExactPing * 0.001f;
			// client has now the most recent ping in seconds
		}
	}
}

// Called to bind functionality to input
void AMainPawn::SetupPlayerInputComponent(class UInputComponent* _InputComponent)
{
	Super::SetupPlayerInputComponent(_InputComponent);

	// action events
	_InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AMainPawn::ZoomIn);
	_InputComponent->BindAction("ZoomIn", IE_Released, this, &AMainPawn::ZoomOut);
	_InputComponent->BindAction("FreeCamera", IE_Pressed, this, &AMainPawn::ActivateFreeCamera);
	_InputComponent->BindAction("FreeCamera", IE_Released, this, &AMainPawn::DeactivateFreeCamera);
	_InputComponent->BindAction("Fire Gun Action", IE_Pressed, this, &AMainPawn::StartGunFire);
	_InputComponent->BindAction("Fire Gun Action", IE_Released, this, &AMainPawn::StopGunFire);
	_InputComponent->BindAction("Fire Missile Action", IE_Pressed, this, &AMainPawn::StartMissileFire);
	_InputComponent->BindAction("Fire Missile Action", IE_Released, this, &AMainPawn::StopMissileFire);
	_InputComponent->BindAction("StopMovement", IE_Pressed, this, &AMainPawn::StopMovement);
	_InputComponent->BindAction("Boost", IE_Pressed, this, &AMainPawn::StartBoost);
	_InputComponent->BindAction("Boost", IE_Released, this, &AMainPawn::StopBoost);
	_InputComponent->BindAction("SwitchTarget", IE_Pressed, this, &AMainPawn::SwitchTargetPressed);
	_InputComponent->BindAction("SwitchTarget", IE_Released, this, &AMainPawn::SwitchTargetReleased);

	// axis events
	_InputComponent->BindAxis("MoveForward", this, &AMainPawn::MoveForward);
	_InputComponent->BindAxis("MoveRight", this, &AMainPawn::MoveRight);
	_InputComponent->BindAction("StrafeLeft", IE_DoubleClick, this, &AMainPawn::MissileEvasionLeft);
	_InputComponent->BindAction("StrafeRight", IE_DoubleClick, this, &AMainPawn::MissileEvasionRight);

	_InputComponent->BindAxis("LookUp", this, &AMainPawn::PitchCamera);
	_InputComponent->BindAxis("LookRight", this, &AMainPawn::YawCamera);


	_InputComponent->BindAction("SkillSlot1", IE_Pressed, this, &AMainPawn::Skill_01_Pressed);
	_InputComponent->BindAction("SkillSlot2", IE_Pressed, this, &AMainPawn::Skill_02_Pressed);
	_InputComponent->BindAction("SkillSlot3", IE_Pressed, this, &AMainPawn::Skill_03_Pressed);
	_InputComponent->BindAction("SkillSlot4", IE_Pressed, this, &AMainPawn::Skill_04_Pressed);
	_InputComponent->BindAction("SkillSlot5", IE_Pressed, this, &AMainPawn::Skill_05_Pressed);
	_InputComponent->BindAction("SkillSlot6", IE_Pressed, this, &AMainPawn::Skill_06_Pressed);
	_InputComponent->BindAction("SkillSlot7", IE_Pressed, this, &AMainPawn::Skill_07_Pressed);
	_InputComponent->BindAction("SkillSlot8", IE_Pressed, this, &AMainPawn::Skill_08_Pressed);
	_InputComponent->BindAction("SkillSlot9", IE_Pressed, this, &AMainPawn::Skill_09_Pressed);
	_InputComponent->BindAction("SkillSlot10", IE_Pressed, this, &AMainPawn::Skill_10_Pressed);

	_InputComponent->BindAction("SkillSlot1", IE_Released, this, &AMainPawn::Skill_01_Released);
	_InputComponent->BindAction("SkillSlot2", IE_Released, this, &AMainPawn::Skill_02_Released);
	_InputComponent->BindAction("SkillSlot3", IE_Released, this, &AMainPawn::Skill_03_Released);
	_InputComponent->BindAction("SkillSlot4", IE_Released, this, &AMainPawn::Skill_04_Released);
	_InputComponent->BindAction("SkillSlot5", IE_Released, this, &AMainPawn::Skill_05_Released);
	_InputComponent->BindAction("SkillSlot6", IE_Released, this, &AMainPawn::Skill_06_Released);
	_InputComponent->BindAction("SkillSlot7", IE_Released, this, &AMainPawn::Skill_07_Released);
	_InputComponent->BindAction("SkillSlot8", IE_Released, this, &AMainPawn::Skill_08_Released);
	_InputComponent->BindAction("SkillSlot9", IE_Released, this, &AMainPawn::Skill_09_Released);
	_InputComponent->BindAction("SkillSlot10", IE_Released, this, &AMainPawn::Skill_10_Released);
}

void AMainPawn::MissileEvasionRight()
{
	RequestEvasiveAction(true);
}

void AMainPawn::MissileEvasionLeft()
{
	RequestEvasiveAction(false);
}

void AMainPawn::RequestEvasiveAction(const bool bDirRight)
{
	if (Role < ROLE_Authority)
	{
		Server_RequestEvasiveAction(bDirRight);
		return;
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::White, "Server received request for evasive action: " + FString(bDirRight ? "Right" : "Left"));
	// Check if evasive action is possible
	if (!GetWorldTimerManager().IsTimerActive(EvasiveActionCoolDown))
	{
		// activate evasive action
		GetWorldTimerManager().SetTimer(EvasiveActionCoolDown, this, &AMainPawn::DoNothing, EvasiveActionCoolDownDuration, false);
		ActivateEvasiveAction(bDirRight);
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "You have to wait " + FString::SanitizeFloat(EvasiveActionCoolDownDuration) + " Seconds between Evasive Actions");
	}
}

void AMainPawn::DoNothing()
{
	// Does absolutely nothing
}

void AMainPawn::Server_RequestEvasiveAction_Implementation(const bool bDirRight)
{
	RequestEvasiveAction(bDirRight);
}

bool AMainPawn::Server_RequestEvasiveAction_Validate(const bool bDirRight)
{
	return true;
}

void AMainPawn::ActivateEvasiveAction(const bool bDirRight)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Request for evasive action granted: " + FString(bDirRight ? "Right" : "Left"));
	GetWorldTimerManager().ClearTimer(EvasiveActionHandle);
	GetWorldTimerManager().SetTimer(EvasiveActionHandle, this, &AMainPawn::DoNothing, 1.0f, false);
}


//Input functions
void AMainPawn::MoveForward(float AxisValue)
{
	if (bBoostPressed)
	{
		MovementInput.X = 1.0f;
	}
	else
	{
		MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
	}
}


void AMainPawn::MoveRight(float AxisValue)
{
	MovementInput.Y = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
}

void AMainPawn::PitchCamera(float AxisValue)
{
	CameraInput.Y = AxisValue;
}

void AMainPawn::YawCamera(float AxisValue)
{
	CameraInput.X = AxisValue;

	//Rotate our actor's yaw, which will turn our camera because we're attached to it
	//{
	//	FRotator NewRotation = GetActorRotation();
	//	NewRotation.Yaw += CameraInput.X;
	//	SetActorRotation(NewRotation);
	//}
}

void AMainPawn::ZoomIn()
{
	bZoomingIn = true;
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(1, 0.0f/*seconds*/, FColor::Red, "ZoomPressed");
#endif
	Camera->FieldOfView = 30.0f;
}

void AMainPawn::ZoomOut()
{
	bZoomingIn = false;
	Camera->FieldOfView = 90.0f;
}

void AMainPawn::ActivateFreeCamera()
{
	bFreeCameraActive = true;
	SpringArm->SetRelativeRotation(FRotator(0, 0, SpringArm->GetRelativeTransform().Rotator().Roll), false, nullptr, ETeleportType::None);
	CurrentSpringArmRotation = SpringArm->GetComponentQuat();
	SpringArm->TargetArmLength = 2500.0f;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 10.0f;
}

void AMainPawn::DeactivateFreeCamera()
{
	bFreeCameraActive = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->TargetArmLength = SpringArmLength;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator, false, nullptr, ETeleportType::None);
}


void AMainPawn::OnRep_GunFire()
{
	if (bGunFire)
	{
		StartGunFire();
	}
	else
	{
		StopGunFire();
	}
}


void AMainPawn::StartGunFire()
{
	// player has gunfire button pressed
	bGunFire = true;
	InputPackage.setGunFire(bGunFire);
	if (bGunReady && bGunHasAmmo)
	{ // gun is ready to fire
		// make sure no other gunfire timer is activ by clearing it
		GetWorldTimerManager().ClearTimer(GunFireHandle);
		// activate a new gunfire timer
		GetWorldTimerManager().SetTimer(GunFireHandle, this, &AMainPawn::GunFire, FireRateGun, true, 0.0f);
#if DEBUG_MSG == 1
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun ON");
#endif
	}
	else if (!GetWorldTimerManager().IsTimerActive(GunFireCooldown))
	{ // gun is not cooling down but could not be fired
		// enable gun
		bGunReady = true;
#if DEBUG_MSG == 1
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun COOLDOWN NOT ACTIVATED");
#endif
		// try again to fire gun
		if (bGunHasAmmo)
		{
			StartGunFire();
		}
#if DEBUG_MSG == 1
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OUT OF AMMO");
		}
#endif
	}
#if DEBUG_MSG == 1
	else
	{
		// gun is cooling down
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun STILL COOLING DOWN");
	}
#endif
}

void AMainPawn::StopGunFire()
{
	// player has gunfire button released
	bGunFire = false;
	InputPackage.setGunFire(bGunFire);
	// is a gunfire timer active
	if (GetWorldTimerManager().IsTimerActive(GunFireHandle))
	{
		// stop the timer
		GetWorldTimerManager().PauseTimer(GunFireHandle);
		// make sure gun is disabled
		bGunReady = false;
		// store the remaining time
		const float CoolDownTime = GetWorldTimerManager().GetTimerRemaining(GunFireHandle);
		// remove old gunfire timer
		GetWorldTimerManager().ClearTimer(GunFireHandle);
		// create a new timer to reactivate gun after a cooldownperiod		
		GetWorldTimerManager().SetTimer(GunFireCooldown, this, &AMainPawn::GunCooldownElapsed, CoolDownTime, false);
	}
#if DEBUG_MSG == 1
	// debug
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OFF");
#endif
}

void AMainPawn::GunCooldownElapsed()
{
#if DEBUG_MSG == 1
	// debug
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "Gun COOLED");
#endif
	// gun has cooled down and is again ready to fire
	bGunReady = true;
	// has the user requested Firing reactivate gunfire
	if (bGunFire)
	{
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun CONTINUE");
#endif
		StartGunFire();
	}
}

void AMainPawn::GunFire()
{
	// start only Firing if there is ammuntion to do so
	if (GunAmmunitionAmount > 0)
	{
		// reset the salve counter
		GunCurrentSalve = 0;
		// start a subtimer that fires GunNumSalves salves
		GetWorldTimerManager().SetTimer(GunSalveTimerHandle, this, &AMainPawn::GunFireSalve, GunSalveIntervall, true, 0.0f);
	}
}

void AMainPawn::GunFireSalve()
{
	// fire a salve only if not all salves have been fired
	if (GunCurrentSalve < GunNumSalves)
	{
		// loop through the projectiles in each salve to be fired
		for (uint8 shot = 0; shot < NumProjectiles; ++shot)
		{
			// stop Firing if there is no ammunition
			if (GunAmmunitionAmount < 1)
			{
				bGunHasAmmo = false;
#if DEBUG_MSG == 1
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "Gun OUT OF AMMO!");
#endif
				GetWorldTimerManager().ClearTimer(GunSalveTimerHandle);
				return;
			}

			// choose next avaliable gun sockets or start over from the first if last was used
			CurrGunSocketIndex = (CurrGunSocketIndex + 1) % GunSockets.Num();
			// get the tranform of the choosen socket
			const FTransform& CurrentSocketTransform = ArmorMesh->GetSocketTransform(GunSockets[CurrGunSocketIndex]);

			const FVector& AdditionalVelocity = ArmorMesh->GetPhysicsLinearVelocityAtPoint(CurrentSocketTransform.GetLocation());

			FVector SpawnDirection;

			// aiming to hit the target
			if (MainLockOnTarget && bHasGunLock)
			{
				// linear targetprediction
				const FVector& TargetLocation = MainLockOnTarget->GetActorLocation();
				const FVector& StartLocation = CurrentSocketTransform.GetLocation();
				const FVector AB = (TargetLocation - StartLocation).GetUnsafeNormal();
				const FVector TargetVelocity = MainTargetVelocity.GetVelocityVector(GetWorld()->GetDeltaSeconds()) - AdditionalVelocity;
				//const FVector TargetVelocity = MainLockOnTarget->GetVelocity() - AdditionalVelocity;
				const FVector vi = TargetVelocity - (FVector::DotProduct(AB, TargetVelocity) * AB);
				GunAimLocation = StartLocation + vi + AB * FMath::Sqrt(FMath::Square(ProjectileVel) - FMath::Pow((vi.Size()), 2.f));

				// get the rotation of the aim-direction
				SpawnDirection = (GunAimLocation - StartLocation).GetUnsafeNormal();
			}
			else
			{
				// get the rotation of the forward vector of the current gun-socket
				SpawnDirection = CurrentSocketTransform.GetRotation().GetForwardVector();
			}

			// add weaponspread
			SpawnDirection = FMath::VRandCone(SpawnDirection, WeaponSpreadRadian);

			// spawn/fire projectile with tracers
			if (TracerIntervall > 0)
			{
				// loop Tracer-Counter
				CurrentTracer = (CurrentTracer + 1) % TracerIntervall;

				// Spawn projectile, if Current-Tracer == 0 a tracer will be visible/spawned
				SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), CurrentTracer == 0, AdditionalVelocity, CurrentSocketTransform.GetLocation());
			}
			else
			{
				// spawn projectiles without tracers
				SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), false, AdditionalVelocity, CurrentSocketTransform.GetLocation());
			}

			// decrease ammunition after each shot
			--GunAmmunitionAmount;

			// add recoil to rootcomponent
			ArmorMesh->AddImpulseAtLocation(SpawnDirection * GunRecoilForce * FMath::FRandRange(0.5f, 1.5f), CurrentSocketTransform.GetLocation());
		}
		// increase salve counter
		++GunCurrentSalve;
	}
	else
	{
		// deactivate the salve-timer if all salves have been fired
		GetWorldTimerManager().ClearTimer(GunSalveTimerHandle);
	}
}

void AMainPawn::SpawnProjectile_Implementation(const FTransform& SocketTransform, const bool bTracer, const FVector& FireBaseVelocity, const FVector& TracerStartLocation)
{
	// method overridden by blueprint to spawn the projectile
}

// Missile spawning START --------------------------------

void AMainPawn::OnRep_MissileFire()
{
	//if (bMissileFire) {
	//	StartMissileFire();
	//}
	//else {
	//	StopMissileFire();
	//}
}

void AMainPawn::StartMissileFire()
{
	// player has Missilefire button pressed
	bMissileFire = true;
	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		InputPackage.setMissileFire(bMissileFire);
		return;
	}

	if (bMissileReady && bMissileHasAmmo)
	{ // Missile is ready to fire
		// make sure no other Missilefire timer is activ by clearing it
		GetWorldTimerManager().ClearTimer(MissileFireHandle);
		// activate a new Missilefire timer
		GetWorldTimerManager().SetTimer(MissileFireHandle, this, &AMainPawn::MissileFire, FireRateMissile, true, 0.0f);
#if DEBUG_MSG == 1
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Missile ON");
#endif
	}
	else if (!GetWorldTimerManager().IsTimerActive(MissileFireCooldown))
	{ // Missile is not cooling down but could not be fired
		// enable Missile
		bMissileReady = true;
#if DEBUG_MSG == 1
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Missile COOLDOWN NOT ACTIVATED");
#endif
		// try again to fire Missile
		if (bMissileHasAmmo)
		{
			StartMissileFire();
		}
#if DEBUG_MSG == 1
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Missile OUT OF AMMO");
		}
#endif
	}
#if DEBUG_MSG == 1
	else
	{
		// Missile is cooling down
		// debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Missile STILL COOLING DOWN");
	}
#endif
}

void AMainPawn::StopMissileFire()
{
	// player has Missilefire button released
	bMissileFire = false;

	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		InputPackage.setMissileFire(bMissileFire);
		return;
	}

	// is a Missilefire timer active
	if (GetWorldTimerManager().IsTimerActive(MissileFireHandle))
	{
		// stop the timer
		GetWorldTimerManager().PauseTimer(MissileFireHandle);
		// make sure Missile is disabled
		bMissileReady = false;
		// store the remaining time
		const float CoolDownTime = GetWorldTimerManager().GetTimerRemaining(MissileFireHandle);
		// remove old Missilefire timer
		GetWorldTimerManager().ClearTimer(MissileFireHandle);
		// create a new timer to reactivate Missile after a cooldownperiod		
		GetWorldTimerManager().SetTimer(MissileFireCooldown, this, &AMainPawn::MissileCooldownElapsed, CoolDownTime, false);
	}
#if DEBUG_MSG == 1
	// debug
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Missile OFF");
#endif
}

void AMainPawn::MissileCooldownElapsed()
{
#if DEBUG_MSG == 1
	// debug
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "Missile COOLED");
#endif
	// Missile has cooled down and is again ready to fire
	bMissileReady = true;
	// has the user requested Firing reactivate Missilefire
	if (bMissileFire)
	{
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Missile CONTINUE");
#endif
		StartMissileFire();
	}
}

void AMainPawn::MissileFire()
{
	if (MissileAmmunitionAmount > 0)
	{
		// reset the salve counter
		MissileCurrentSalve = 0;
		// the Missilefire timer starts a subtimer that fires all the salves
		GetWorldTimerManager().SetTimer(MissileSalveTimerHandle, this, &AMainPawn::MissileFireSalve, MissileSalveIntervall, true, 0.0f);
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, FString::SanitizeFloat(MissileSalveIntervall) + " SalveDelta; " + FString::SanitizeFloat(FireRateMissile) + " FireDelta ");
#endif
	}
}

void AMainPawn::MissileFireSalve()
{
	if (MissileCurrentSalve < MissileNumSalves)
	{
		for (uint8 shot = 0; shot < NumMissiles; ++shot)
		{
			// choose next avaliable Missile sockets or start over from the first if last was used
			CurrMissileSocketIndex = (CurrMissileSocketIndex + 1) % MissileSockets.Num();
			// get the tranform of the choosen socket
			const FTransform& CurrentSocketTransform = ArmorMesh->GetSocketTransform(MissileSockets[CurrMissileSocketIndex], ERelativeTransformSpace::RTS_World);

			// calculate a direction
			FVector SpawnDirection = CurrentSocketTransform.GetRotation().GetForwardVector();

			const FVector& AdditionalVelocity = ArmorMesh->GetPhysicsLinearVelocityAtPoint(CurrentSocketTransform.GetLocation());

			USceneComponent* HomingTarget = nullptr;

			if (bMultiTarget && MultiTargets.Num() > 0)
			{
				CurrTargetIndex = (CurrTargetIndex + 1) % MultiTargets.Num();
			}

			if (bMultiTarget && MultiTargets.Num() > 0 && MultiTargets.IsValidIndex(CurrTargetIndex) && MultiTargets[CurrTargetIndex])
			{
				HomingTarget = MultiTargets[CurrTargetIndex]->GetRootComponent();
				// calculate a direction and apply weaponspread
				SpawnDirection = FMath::VRandCone(SpawnDirection, MissileSpreadRadian);
			}
			else
			{
				CurrTargetIndex = 0;
				if (MainLockOnTarget && bHasMissileLock)
				{
					HomingTarget = MainLockOnTarget->GetRootComponent();
					// calculate a direction and apply weaponspread
					SpawnDirection = FMath::VRandCone(SpawnDirection, MissileSpreadRadian);
				}
			}
			SpawnMissile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), HomingTarget, AdditionalVelocity);

			// decrease ammunition
			--MissileAmmunitionAmount;

			if (MissileAmmunitionAmount > 0)
			{
#if DEBUG_MSG == 1
				// debug
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, "Missiles left: " + FString::FromInt(MissileAmmunitionAmount));
#endif
			}
			else
			{
				bMissileHasAmmo = false;
#if DEBUG_MSG == 1
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "Missile OUT OF AMMO!");
#endif
				GetWorldTimerManager().ClearTimer(MissileSalveTimerHandle);
			}
		}
		++MissileCurrentSalve;
		return;
	}
	// deactivate the salvetimer
	GetWorldTimerManager().ClearTimer(MissileSalveTimerHandle);
}

void AMainPawn::SpawnMissile_Implementation(const FTransform& SocketTransform, class USceneComponent* HomingTarget, const FVector& FireBaseVelocity)
{
	// method overridden by blueprint to spawn the projectile
}

// Missile spawning END ----------------------------------


void AMainPawn::StopMovement()
{
	StopPlayerMovement();
}

void AMainPawn::StopPlayerMovement()
{
	Server_StopPlayerMovement();
}

bool AMainPawn::Server_StopPlayerMovement_Validate()
{
	return true;
}

void AMainPawn::Server_StopPlayerMovement_Implementation()
{
	if (bCanReceivePlayerInput)
	{
		bGunReady = false;
		bMissileReady = false;
		// make sure there is no pending movement activation
		GetWorldTimerManager().ClearTimer(StartMovementTimerHandle);
		bCanReceivePlayerInput = false;
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "Movement STOPPED");
#endif
	}
	else
	{
		GetWorldTimerManager().SetTimer(StartMovementTimerHandle, this, &AMainPawn::StartMovementCoolDownElapsed, 1.0f, false);
	}
}

void AMainPawn::StartMovementCoolDownElapsed()
{
	bGunReady = true;
	bMissileReady = true;
	bCanReceivePlayerInput = true;
	RotControlStrength = TimeOfNoControl;
	MovControlStrength = TimeOfNoControl;
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Movement STARTED");
#endif
}


void AMainPawn::StartBoost()
{
	bBoostPressed = true;
}

void AMainPawn::StopBoost()
{
	bBoostPressed = false;
}

void AMainPawn::SwitchTargetPressed()
{
	if (bSwitchTargetPressed) return;

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Switch Target PRESSED");

	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		InputPackage.setSwitchTarget(true);
		return;
	}


	bSwitchTargetPressed = true;
	MainLockOnTarget = nullptr;
	MultiTargets.Empty();
	bContinuousLockOn = false;
	bLockOnDelayActiv = false;
	GetWorldTimerManager().ClearTimer(ContinuousLockOnDelay);
}

void AMainPawn::SwitchTargetReleased()
{
	if (Role < ROLE_Authority && IsLocallyControlled())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Switch Target RELEASED");
		InputPackage.setSwitchTarget(false);
		return;
	}
	bSwitchTargetPressed = false;
}


// sends Playerinput to server
void AMainPawn::GetPlayerInput(FPlayerInputPackage inputData)
{
	Server_GetPlayerInput(inputData);
}

bool AMainPawn::Server_GetPlayerInput_Validate(FPlayerInputPackage inputData)
{
	return true;
}

//Server receives Input
void AMainPawn::Server_GetPlayerInput_Implementation(FPlayerInputPackage inputData)
{
	if (GetWorld())
	{
		NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
		lastUpdate = GetWorld()->RealTimeSeconds;
#if DEBUG_MSG == 1
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 0/*seconds*/, FColor::Green,
			                                 FString::SanitizeFloat(NetDelta) + "    " +
			                                 FString::FromInt(GetVelocity().Size() * 0.036f) + " km/h");
#endif
	}

	if (inputData.getPacketNumber() > Ack)
	{
		Ack = inputData.getPacketNumber();
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "accepting Packet = " + FString::FromInt(inputData.getPacketNumber()));
#endif
		InputPackage = inputData;
	}
	else
	{
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Red, "Packet not Accepted = " + FString::FromInt(inputData.getPacketNumber()));
#endif
		return;
	}


	if (InputPackage.getGunFire())
	{
		if (!bGunFire)
		{
			StartGunFire();
		}
	}
	else
	{
		if (bGunFire)
		{
			StopGunFire();
		}
	}

	if (InputPackage.getMissileFire())
	{
		if (!bMissileFire)
		{
			StartMissileFire();
		}
	}
	else
	{
		if (bMissileFire)
		{
			StopMissileFire();
		}
	}

	if (InputPackage.getSwitchTarget())
	{
		SwitchTargetPressed();
	}
	else
	{
		SwitchTargetReleased();
	}

	bHasGunLock = InputPackage.getGunLock();
	bHasMissileLock = InputPackage.getMissileLock();

	if (GEngine && bGunFire) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "Received GunfireInput");
	if (GEngine && bMissileFire) GEngine->AddOnScreenDebugMessage(-1, NetDelta, FColor::Green, "Received GunfireInput");


	AuthorityAck = Ack;
}

void AMainPawn::OnRep_AuthorityAck()
{
	int DeletionCnt = 0;
	PastClientTransform = FTransform();
	while (true)
	{
		if (MovementHistory.Num() > 0)
		{
			if (MovementHistory[0].PacketNo < AuthorityAck)
			{
				MovementHistory.RemoveAt(0);
				++DeletionCnt;
				continue;
			}
			if (MovementHistory[0].PacketNo == AuthorityAck)
			{
				PastClientTransform = MovementHistory[0].Transform;
				MovementHistory.RemoveAt(0);
				++DeletionCnt;
				break;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f / NetUpdateFrequency, FColor::Red, "Deleted " + FString::FromInt(DeletionCnt) + " old Transforms, pending to be approved : " + FString::FromInt(MovementHistory.Num()));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f / NetUpdateFrequency, FColor::Green, "Last acceptet Packet = " + FString::FromInt(AuthorityAck));
#endif
	this->Ack = AuthorityAck;
}

inline void AMainPawn::GetCursorLocation(FVector2D& _CursorLoc)
{
	if (GetController())
	{
		APlayerController* controller = Cast<APlayerController>(GetController());
		if (controller)
		{
			controller->GetMousePosition(_CursorLoc.X, _CursorLoc.Y);
#if DEBUG_MSG == 1
			if (GEngine)
				GEngine->AddOnScreenDebugMessage(-1, 0/*seconds*/, FColor::Red,
				                                 FString::SanitizeFloat(_CursorLoc.X) + " " +
				                                 FString::SanitizeFloat(_CursorLoc.Y));
#endif
		}
	}
}

inline void AMainPawn::GetViewportSizeCenter(FVector2D& _ViewPortSize, FVector2D& _ViewPortCenter)
{
	if (GetWorld())
	{
		if (GetWorld()->GetGameViewport())
		{
			GetWorld()->GetGameViewport()->GetViewportSize(_ViewPortSize);
			_ViewPortCenter = _ViewPortSize * 0.5f;
		}
	}
}

inline void AMainPawn::GetMouseInput(FVector2D& _MouseInput, FVector2D& _CursorLoc, FVector2D& _ViewPortCenter)
{
	{
		_MouseInput = (_CursorLoc - _ViewPortCenter) / _ViewPortCenter;
		_MouseInput *= _MouseInput.GetSafeNormal().GetAbsMax();
		// deadzone (5 pixel)
		if (_MouseInput.Size() < (5.0f / ViewPortSize.X)) _MouseInput = FVector2D::ZeroVector;
	}
}


void AMainPawn::WeaponLock()
{
	if (MainLockOnTarget)
	{
		// update the helper struct
		MainTargetVelocity.SetCurrentLocation(MainLockOnTarget->GetActorLocation());

		// normalized Forward-Vector from the Armor or the Camera, depending on where the player is looking, to prevent locking onto Targets that are not in front of the Player
		const FVector& ForwardVector = bFreeCameraActive ? ArmorMesh->GetForwardVector() : Camera->GetForwardVector();
		const FVector DirToTarget = bFreeCameraActive ? (MainLockOnTarget->GetActorLocation() - GetActorLocation()).GetUnsafeNormal() : (MainLockOnTarget->GetActorLocation() - Camera->GetComponentLocation()).GetUnsafeNormal();

		// angle between Forward-Vector and Vector to current Target
		const float DeltaAngleRad = FVector::DotProduct(ForwardVector, DirToTarget);

		// set the booleans to enable Weapon-Lock-Ons
		bHasMissileLock = (DeltaAngleRad > MissileLockOnAngleRad) ? true : false;
		bHasGunLock = (DeltaAngleRad > GunLockOnAngleRad) ? true : false;

		InputPackage.setGunLock(bHasGunLock);
		InputPackage.setMissileLock(bHasMissileLock);
	}
}


void AMainPawn::TargetLock()
{
	// execute on locally controlled instance of pawn	
	if (bSwitchTargetPressed && !GetWorldTimerManager().IsTimerActive(ContinuousLockOnDelay))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "-----------------------------------------------------------------------");
		MainLockOnTarget = nullptr;
	}

	// normalized Forward-Vector from the Armor or the Camera, depending on where the player is looking, to prevent locking onto Targets that are not in front of the Player
	const FVector& ForwardVector = bFreeCameraActive ? ArmorMesh->GetForwardVector() : Camera->GetForwardVector();

	// skip Target-Selection when the delay is active or there has already been Locked-on to are Target and Multi-Target is not enabled
	if (bLockOnDelayActiv || (MainLockOnTarget && !bMultiTarget))
	{
		return;
	}
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "... looking for targets");
#endif

	TMap<float, AActor*> TargetableTargets;

	// get all target-able Actors
	for (TActorIterator<AActor> currActor(GetWorld()); currActor /* while is valid */; ++currActor)
	{
		if (*currActor != this && currActor->Implements<UTarget_Interface>())
		{
#if DEBUG_MSG == 1
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, currActor->GetName() + " has the Interface");
#endif

			// Vector to the current target-able Target
			FVector VecToTarget;			
			if (currActor && Camera) {
				if(bFreeCameraActive)
				{
					VecToTarget = currActor->GetActorLocation() - GetActorLocation();
				}
				else
				{
					VecToTarget = currActor->GetActorLocation() - Camera->GetComponentLocation();
				}				
			}
			
			// angle between Forward-Vector and Vector to current Target
			const float DeltaAngleRad = FVector::DotProduct(ForwardVector, VecToTarget.GetSafeNormal());

			// check whether the Actor is in Radar-Range
			if (VecToTarget.SizeSquared() > RadarMaxLockOnRangeSquarred)
			{
				continue;
			}

			// add to list of TargetableTargets if in Gun-Lock-On-Range or when Multi-Targeting is enabled in Multi-Target-Lock-On-Range
			if (DeltaAngleRad > GunLockOnAngleRad || (bMultiTarget && DeltaAngleRad > MultiTargetLockOnAngleRad))
			{
				TargetableTargets.Add(DeltaAngleRad, *currActor);
			}
		}
	}

	// sort the Targets
	TargetableTargets.KeySort([](const float A, const float B)
		{
			return (A - B) > 0.0f;
		});


	TArray<AActor*> ChosenTargets;

	int TargetCount = 0;
	// copy only MaxNumTargets to the Chosen-Targets-List
	for (const auto& entry : TargetableTargets)
	{
		if (TargetCount < MaxNumTargets)
		{
			ChosenTargets.Add(entry.Value);
			++TargetCount;
		}
		else
		{
			break;
		}
	}


	// set the main target
	if (!MainLockOnTarget)
	{
		// set the main target to be the first element of the target-list
		MainLockOnTarget = ChosenTargets.IsValidIndex(0) ? ChosenTargets[0] : nullptr;
		// initialize the helper struct for velocity calculation
		if (MainLockOnTarget)
		{
			MainTargetVelocity.Init(MainLockOnTarget->GetActorLocation());
		}
	}

	// activate the delay for continuous LockOn
	if (MainLockOnTarget && !bContinuousLockOn)
	{
		GetWorldTimerManager().SetTimer(ContinuousLockOnDelay, this, &AMainPawn::ActivateContinueousLockOn, 0.5f, false);
	}

	// empty the list if multi-targeting is disabled
	if (!bMultiTarget)
	{
		MultiTargets.Empty();
	}

	// flag to keep track of a change in the list
	bool bMultiTargetListChanged = false;
	// check whether the list of targets has to be sent to the server by looking for changes
	for (AActor* actor : ChosenTargets)
	{
		// check if the actor is already being targeted
		if (actor && MultiTargets.Contains(actor))
		{
			continue;
		}
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Actor List changed");
#endif
		// if not the target list need to be updated: a change has occured
		bMultiTargetListChanged = true;
		break;
	}

	// in case the list has changed update target-list and send the new list to the server
	if (bMultiTargetListChanged || MultiTargets.Num() != ChosenTargets.Num())
	{
#if DEBUG_MSG == 1
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "Number of additionally targeted actors: " + FString::FromInt(ChosenTargets.Num()));
#endif
		// update the Multi-Targets-Array
		MultiTargets = ChosenTargets;
		// send the new Array to the server
		SetTargets(MainLockOnTarget, MultiTargets);
	}
}

void AMainPawn::ActivateContinueousLockOn()
{
	bContinuousLockOn = true;
}

void AMainPawn::OnRep_MultiTarget()
{
	// Multitarget activated
	// TODO: implemenation
}

void AMainPawn::SetTargets(AActor* MainTarget, const TArray<AActor*>& OtherTargets)
{
	if (Role < ROLE_Authority)
	{
		Server_SetTargets(MainTarget, OtherTargets);
		return;
	}

	MainLockOnTarget = MainTarget;
	MultiTargets = OtherTargets;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, "Server received number of Targets by player : " + FString::FromInt(OtherTargets.Num()));
}

void AMainPawn::Server_SetTargets_Implementation(AActor* MainTarget, const TArray<AActor*>& OtherTargets)
{
	SetTargets(MainTarget, OtherTargets);
}

bool AMainPawn::Server_SetTargets_Validate(AActor* MainTarget, const TArray<AActor*>& OtherTargets)
{
	return true;
}

void AMainPawn::Skill_01_Pressed()
{
}

void AMainPawn::Skill_02_Pressed()
{
}

void AMainPawn::Skill_03_Pressed()
{
}

void AMainPawn::Skill_04_Pressed()
{
}

void AMainPawn::Skill_05_Pressed()
{
}

void AMainPawn::Skill_06_Pressed()
{
}

void AMainPawn::Skill_07_Pressed()
{
}

void AMainPawn::Skill_08_Pressed()
{
}

void AMainPawn::Skill_09_Pressed()
{
}

void AMainPawn::Skill_10_Pressed()
{
}

void AMainPawn::Skill_01_Released()
{
}

void AMainPawn::Skill_02_Released()
{
}

void AMainPawn::Skill_03_Released()
{
}

void AMainPawn::Skill_04_Released()
{
}

void AMainPawn::Skill_05_Released()
{
}

void AMainPawn::Skill_06_Released()
{
}

void AMainPawn::Skill_07_Released()
{
}

void AMainPawn::Skill_08_Released()
{
}

void AMainPawn::Skill_09_Released()
{
}

void AMainPawn::Skill_10_Released()
{
}


/* TESTING
AActor * ClosestActor = nullptr;
	float DistanceToClosestActor = BIG_NUMBER;
	for (TActorIterator<AActor> currActor(GetWorld()); currActor; ++currActor) {
		if (*currActor == this || (currActor->GetOwner() && currActor->GetOwner() == this) || (currActor->GetInstigator() == GetInstigator())) continue;
		float Distance = currActor->GetDistanceTo(this);
		if (Distance < DistanceToClosestActor) {
			DistanceToClosestActor = Distance;
			ClosestActor = *currActor;
		}
	}
	if (ClosestActor) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, ClosestActor->GetName() + " : " + FString::SanitizeFloat(DistanceToClosestActor / 100) + " m");
	}


	USceneComponent * ClosestSceneComponent = nullptr;
	float DistanceToClosestSceneComponent = BIG_NUMBER;

	for (TObjectIterator<USceneComponent> Itr; Itr; ++Itr)
	{
		if (Itr->GetOwner() && Itr->GetOwner() != this) {
			float Distance = Itr->GetOwner()->GetDistanceTo(this);
			if (Distance < DistanceToClosestSceneComponent) {
				DistanceToClosestSceneComponent = Distance;
				ClosestSceneComponent = *Itr;
			}
		}
	}
	if (ClosestSceneComponent) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, ClosestSceneComponent->GetName() + " : " + FString::SanitizeFloat(DistanceToClosestSceneComponent / 100) + " m");
	}
	*/