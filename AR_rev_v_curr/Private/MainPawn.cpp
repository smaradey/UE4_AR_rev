// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "MainPawn.h"


// Sets default values
AMainPawn::AMainPawn(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = false;
	bReplicateMovement = false;


	//SetActorEnableCollision(true);

	//Create components
	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create static mesh component
	ArmorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmorMesh"));
	RootComponent = ArmorMesh;
	ArmorMesh->OnComponentHit.AddDynamic(this, &AMainPawn::ArmorHit);
	//ArmorMesh->OnComponentHit.AddDynamic<AMainPawn>(this, &AMainPawn::ArmorHit);
	//MissileMesh->OnComponentBeginOverlap.AddDynamic(this, &AMissile::MissileMeshOverlap);


	Dummy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Root"));

	//RootComponent = Root;

	Dummy->AttachTo(ArmorMesh, NAME_None);


	//ArmorMesh->AttachTo(RootComponent);
	ArmorMesh->SetCollisionObjectType(ECC_Pawn);
	ArmorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArmorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ArmorMesh->SetSimulatePhysics(true);
	ArmorMesh->SetEnableGravity(false);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	SpringArm->AttachTo(RootComponent, NAME_None);
	//SpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 50.0f), FRotator(0.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 0.0f;
	SpringArmLength = SpringArm->TargetArmLength;
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 100.0f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 8.0f;
	SpringArm->CameraLagMaxDistance = 2500.0f;


	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	//Camera->AttachTo(RootComponent);
	Camera->AttachTo(SpringArm, USpringArmComponent::SocketName);
	// PostProcessSettings
	Camera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	Camera->PostProcessSettings.LensFlareIntensity = 0.0f;
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = 0.5f;
	Camera->PostProcessSettings.bOverride_AntiAliasingMethod = true;
	Camera->PostProcessSettings.AntiAliasingMethod = EAntiAliasingMethod::AAM_TemporalAA;
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.2f;

	GunSockets.Add("gun0");
	GunSockets.Add("gun1");



	//Take control of the default Player
	//AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AMainPawn::ArmorHit(class AActor* OtherActor, class UPrimitiveComponent * OtherComponent, FVector Loc, const FHitResult& FHitResult) {
	CollisionHandling = true;
	// location and normal for plane generation
	MostRecentCrashPoint = FHitResult.Location;
	CrashNormal = FHitResult.Normal;
}

void AMainPawn::RecoverFromCollision(const float DeltaSeconds) {
	if (CollisionHandling && bCanReceivePlayerInput) {

		// orthogonal distance to plane where collision happened
		const float CurrDistanceToColl = FVector::PointPlaneDist(GetActorLocation(), MostRecentCrashPoint, CrashNormal);
		// distance from safety distance (20m)
		float DeltaDistance = 2000.0f - CurrDistanceToColl;		
		// factor to slow down the vehicle to prevent overshooting the point of safedistance
		const float Derivative = (DeltaDistance - PrevSafetyDistanceDelta) / PrevDeltaTime;
		// Velocity that will be added to the vehicle in order to get away from the collision point
		AntiCollisionVelocity = (DeltaDistance + 0.5f*Derivative) * CollisionTimeDelta * 0.01f;
		// apply the the impuls away from the collision point
		ArmorMesh->AddImpulse(CrashNormal * AntiCollisionVelocity, NAME_None, true);
		// add the elapsed time
		CollisionTimeDelta += DeltaSeconds;
		// reset the collision handling system when either:
		// - the timelimit has been passed
		// - half of the distance to safe distance has been passed
		// - the vehicle got lower than the original collisionpoint
		if (CollisionTimeDelta > TimeOfAntiCollisionSystem || DeltaDistance < 0.0f || CurrDistanceToColl > 2000.0f) {
			CollisionHandling = false;
			AntiCollisionVelocity = 0.0f;
			CollisionTimeDelta = 0.0f;
			PrevSafetyDistanceDelta = 0.0f;
		}
		// store data for next tick to calculate the breakfactor for slowing down
		PrevDeltaTime = DeltaSeconds;
		PrevSafetyDistanceDelta = DeltaDistance;
	}
}
void AMainPawn::InitWeapon(){
	WeaponSpreadRadian = WeaponSpreadHalfAngle * PI / 180.0f;
	SalveIntervall = (SalveDensity * FireRateGun) / NumSalves;
	bHasAmmo = GunAmmunitionAmount > 0;
}

// Called when the game starts or when spawned
void AMainPawn::BeginPlay() {
	Super::BeginPlay();

	InitWeapon();

	lastUpdate = GetWorld()->RealTimeSeconds;
	PrevReceivedTransform = GetTransform();
	CalculateVelocityDeltas();

	if (GetNetMode() == NM_Client) {
		// deactivate physics on clients
		ArmorMesh->SetSimulatePhysics(false);
		NumberOfBufferedNetUpdates = FMath::RoundToInt(Smoothing * NetUpdateFrequency);
		NumberOfBufferedNetUpdates = FMath::Max(NumberOfBufferedNetUpdates, 2);
		LerpVelocity = NetUpdateFrequency / NumberOfBufferedNetUpdates;
		PredictionAmount = (NumberOfBufferedNetUpdates + AdditionalUpdatePredictions) / NetUpdateFrequency;
		// TODO: check if working, eventually call from server	
		if(IsLocallyControlled()){
			SetAutonomousProxy(true);
		}		


	}

}

// Called every frame
void AMainPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	CurrentVelocitySize = GetVelocity().Size();

	if (IsLocallyControlled()) {

		if (bUseInternMouseSensitivity) {
			InputAxis += CameraInput * MouseSensitivity;
		}
		else {
			// get mouse position
			GetCursorLocation(CursorLoc);

			// get viewport size/center
			GetViewportSizeCenter(ViewPortSize, ViewPortCenter);
			InputAxis = (CursorLoc - ViewPortCenter) / ViewPortCenter;
		}
		// clamp the axis to make sure the player can't turn more than maxTurnrate
		InputAxis.X = FMath::Clamp(InputAxis.X, -1.0f, 1.0f);
		InputAxis.Y = FMath::Clamp(InputAxis.Y, -1.0f, 1.0f);
		MouseInput = InputAxis * InputAxis.GetSafeNormal().GetAbsMax();

		// debug
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat(InputAxis.X) + " x " + FString::SanitizeFloat(InputAxis.Y));
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat((InputAxis * InputAxis.GetSafeNormal().GetAbsMax()).Size()));

		// deadzone with no turning -> mouseinput interpreted as zero
		if (MouseInput.SizeSquared() < Deadzone*Deadzone) {
			MouseInput = FVector2D::ZeroVector;
		}
		else {
			// converting the mouseinput to use more precise turning around the screencenter
			MouseInput.X = (1.0f - FMath::Cos(InputAxis.X * HalfPI)) * FMath::Sign(InputAxis.X);
			MouseInput.Y = (1.0f - FMath::Cos(InputAxis.Y * HalfPI))* FMath::Sign(InputAxis.Y);


			// lerping with the customizable Precisionfactor
			MouseInput.X = FMath::Lerp(InputAxis.X, MouseInput.X, CenterPrecision);
			MouseInput.Y = FMath::Lerp(InputAxis.Y, MouseInput.Y, CenterPrecision);

			MouseInput *= MouseInput.GetSafeNormal().GetAbsMax();
		}

		if (bFreeCameraActive) {
			// rotation from mousemovement (input axis lookup and lookright)
			CurrentSpringArmRotation = CurrentSpringArmRotation * FQuat(FRotator(CameraInput.Y * -FreeCameraSpeed, CameraInput.X * FreeCameraSpeed, 0.0f));
			// rotate the camera with the springarm

			SpringArm->SetWorldRotation(CurrentSpringArmRotation);

			//SpringArm->SetWorldRotation((AutoLevelAxis.Rotation().Quaternion() * CurrentSpringArmRotation));

			// disable rotationcontrol
			MouseInput = FVector2D::ZeroVector;
			// disable strafeinput
			MovementInput.Y = 0.0f;
		}

		if (!bCanReceivePlayerInput) {
			MovementInput = MouseInput = FVector2D::ZeroVector;
		}

		InputPackage.PacketNo++;

		FInput currentInput;
		currentInput.PacketNo = InputPackage.PacketNo;
		currentInput.MouseInput = MouseInput;
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(MouseInput.X) + " x " + FString::SanitizeFloat(MouseInput.Y));
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(MouseInput.Size()));
		currentInput.MovementInput = MovementInput;

		while (InputPackage.InputDataList.Num() >= 1) {
			InputPackage.InputDataList.RemoveAt(0);
		}

		InputPackage.InputDataList.Add(currentInput);
		InputPackage.Ack = Ack;

		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, "Length of Array = " + FString::FromInt(InputPackage.InputDataList.Num()));

		GetPlayerInput(InputPackage);


	}

	switch (Role) {
		case ROLE_Authority:
			{
				FVector2D PrevUsedMouseInput = PreviousMouseInput;

				if (InputPackage.InputDataList.Num() > 0) {

					const FInput &currentInput = InputPackage.InputDataList.Last();


					MouseInput = InputPackage.InputDataList.Last().MouseInput;

					MovementInput = InputPackage.InputDataList.Last().MovementInput;

				}
				// keep a copy of the mouseinput
				const FVector2D RawMouseInput = MouseInput;

				if (!bCanReceivePlayerInput) {
					MovementInput = MouseInput = FVector2D::ZeroVector;
				}

				{
					// yaw smoothing
					if (PrevUsedMouseInput.X * MouseInput.X > 0.0f
							&& MouseInput.X * MouseInput.X < PrevUsedMouseInput.X * PrevUsedMouseInput.X) // new x is smaller
					{
						MouseInput.X = FMath::FInterpTo(PrevUsedMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed * ResetSpeed);
					}
					else {
						MouseInput.X = FMath::FInterpTo(PrevUsedMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed);
					}

					// pitch smoothing
					if (PrevUsedMouseInput.Y * MouseInput.Y > 0.0f
							&& MouseInput.Y * MouseInput.Y < PrevUsedMouseInput.Y * PrevUsedMouseInput.Y) // new y is smaller
					{
						MouseInput.Y = FMath::FInterpTo(PrevUsedMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed * ResetSpeed);
					}
					else {
						MouseInput.Y = FMath::FInterpTo(PrevUsedMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed);
					}
				}

				// player is flying and not stopped
				if (bCanReceivePlayerInput) {
					// Forward Velocity during flight
					if (MovementInput.X > 0.0f) {
						// forward
						ForwardVel = FMath::FInterpTo(ForwardVel, MovementInput.X * VelForwardDelta + DefaultForwardVel, DeltaTime, ForwardAcceleration);
					}
					else {
						// backwards
						ForwardVel = FMath::FInterpTo(ForwardVel, MovementInput.X * VelBackwardsDelta + DefaultForwardVel, DeltaTime, BackwardsAcceleration);
					}
					// Strafe Velocity during flight
					if (bUseConstantStrafeAcceleration) {
						if (MovementInput.Y == 0.0f) {
							StrafeVel = FMath::FInterpTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, StrafeBankAcceleration);
						}
						else {
							StrafeVel = FMath::FInterpConstantTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, ConstantStrafeAcceleration);
						}
					}
					else {
						StrafeVel = FMath::FInterpTo(StrafeVel, MovementInput.Y * MaxStrafeVel, DeltaTime, StrafeBankAcceleration);
					}
				}
				else {
					// player has not input/has stopped
					ForwardVel = FMath::FInterpTo(ForwardVel, 0.0f, DeltaTime, BackwardsAcceleration);
					StrafeVel = FMath::FInterpTo(StrafeVel, 0.0f, DeltaTime, StrafeBankAcceleration);
				}

				// after a collision disable playerinput for a specified amount of time
				if (MovControlStrength < TimeOfNoControl) {
					ForwardVel = StrafeVel = 0.0f;
				}

				// select new bankrotation either from strafe input or from current turnvalue 
				if (MovementInput.Y != 0) {
					// rot from strafeinput
					CurrStrafeRot = FMath::FInterpTo(PrevStrafeRot, MovementInput.Y * -MaxStrafeBankAngle, DeltaTime, StrafeBankAcceleration);
				}
				else {
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

					// print current absolut turnrate (angular velocity)
					if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::SanitizeFloat(WorldAngVel.Size()) + " deg/sec");

					// collisionhandling: is set to zero on "each" collision and recovers in 2 seconds
					RotControlStrength = FMath::FInterpConstantTo(RotControlStrength, TimeOfNoControl + 1.0f, DeltaTime, 1.0f);

					// if 1 second has passed and not yet fully recovered
					if (RotControlStrength > TimeOfNoControl && RotControlStrength < TimeOfNoControl + 1.0f) {
						ArmorMesh->SetAngularDamping(0.0f);
						const float Alpha = FMath::Square(RotControlStrength - TimeOfNoControl);
						if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Orange, "ROTATION control CHARGING " + FString::SanitizeFloat(Alpha));

						const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime, 0.0f, 0.0f));

						// blend between pure physics velocities and player caused velocity				
						const FVector NewAngVel = FMath::Lerp(ArmorMesh->GetPhysicsAngularVelocity(), WorldAngVel - AngVelStrafeCompensation, Alpha);
						ArmorMesh->SetPhysicsAngularVelocity(NewAngVel);

						// rotate springarm/camera in local space to compensate for straferotation 
						if (!bFreeCameraActive) {
							SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
						}
					}
					// no collision handling (normal flight)
					else if (RotControlStrength >= TimeOfNoControl + 1.0f) {
						if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, "ROTATION control FULL");

						// auto level function  
						if (LevelVel > 0.0f && !bFreeCameraActive) {
							// TODO: get rid of Acos
							// dot product between gravity vector and actor rightvector
							const float DotUpRight = FVector::DotProduct(Camera->GetRightVector(), bUseGravityDirForAutoLevel ? AutoLevelAxis : FVector(0.0f, 0.0f, 1.0f));
							const float LevelHorizonVel = LevelVel * (90.0f - FMath::Acos(DotUpRight) * 180.0f / PI);

							const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime + LevelHorizonVel, 0, 0));

							// player input is directly translated into movement
							ArmorMesh->SetPhysicsAngularVelocity(WorldAngVel - AngVelStrafeCompensation);
							// rotate springarm/camera in local space to compensate for straferotation 
							SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot + LevelHorizonVel * DeltaTime), false, nullptr, ETeleportType::None);
						}
						else {
							const FVector AngVelStrafeCompensation = GetActorRotation().RotateVector(FVector(DeltaRot / DeltaTime, 0, 0));

							// player input is directly translated into movement
							ArmorMesh->SetPhysicsAngularVelocity(WorldAngVel - AngVelStrafeCompensation);
							// rotate springarm/camera in local space to compensate for straferotation 
							if (!bFreeCameraActive) {
								SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
							}
						}
					}
					else {
						// RotControlStrength is between 0 and TimeOfNoControl -> player has no control: collision or input disabled
						ArmorMesh->SetAngularDamping(5.0f);
						if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "ROTATION control DEACTIVATED");
						if (!bFreeCameraActive) {
							SpringArm->SetRelativeRotation(FRotator(0, 0, CurrStrafeRot), false, nullptr, ETeleportType::None);
						}
					}
				}

				// location
				{
					TargetLinearVelocity = GetActorForwardVector() * ForwardVel + GetActorRightVector() * StrafeVel;

					// straferotation compensation 
					TargetLinearVelocity = TargetLinearVelocity.RotateAngleAxis(-CurrStrafeRot, GetActorForwardVector());

					MovControlStrength = FMath::FInterpConstantTo(MovControlStrength, TimeOfNoControl + 1.0f, DeltaTime, 1.0f);

					// return the control to the player over a time of 1 second
					if (MovControlStrength > TimeOfNoControl && MovControlStrength < TimeOfNoControl + 1.0f) {
						ArmorMesh->SetPhysicsLinearVelocity(FMath::Lerp(ArmorMesh->GetPhysicsLinearVelocity(), TargetLinearVelocity, MovControlStrength - TimeOfNoControl));
					}
					else if (MovControlStrength >= TimeOfNoControl + 1.0f) {
						// normal flight: set velocities directly
						ArmorMesh->SetPhysicsLinearVelocity(TargetLinearVelocity);
					}
					// move away from collision location
					RecoverFromCollision(DeltaTime);
				}

				// movement replication
				{
					TransformOnAuthority = GetTransform();
					LinearVelocity = ArmorMesh->GetPhysicsLinearVelocity();
					AngularVelocity = ArmorMesh->GetPhysicsAngularVelocity();
				}

				// preparation for next tick
				PreviousMouseInput = MouseInput;
			}
			break;

		case ROLE_SimulatedProxy:
			{
				// proxyclient movement
				LerpProgress += DeltaTime;
				float convertedLerpFactor = FMath::Clamp(LerpProgress * LerpVelocity, 0.0f, 1.0f);
				FTransform NewTransform;
				NewTransform.Blend(TransformOnClient, TargetTransform, convertedLerpFactor);
				// transform actor to new location/rotation
				SetActorTransform(NewTransform, false, nullptr, ETeleportType::None);

			}
			break;

		case ROLE_AutonomousProxy:
			{

				GetPing();
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "Ping = " + FString::SanitizeFloat(Ping) + " s");
				//FTransform newTransform;

				//newTransform.Blend(TransformOnClient, TransformOnAuthority, FMath::Min(Alpha * NetUpdateFrequency*0.9f, 1.0f)); //?
				//Alpha += DeltaTime;
				//SetActorTransform(newTransform);
			}
			break;

		default:
			{

			}
	}
}


// replication of variables
void AMainPawn::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> &OutLifetimeProps) const {
	DOREPLIFETIME(AMainPawn, bGunReady);
	DOREPLIFETIME(AMainPawn, TransformOnAuthority);
	DOREPLIFETIME(AMainPawn, LinearVelocity);
	DOREPLIFETIME(AMainPawn, AngularVelocity);
	DOREPLIFETIME(AMainPawn, WorldAngVel);
	DOREPLIFETIME(AMainPawn, TargetLinearVelocity);
	DOREPLIFETIME(AMainPawn, bCanReceivePlayerInput);
	DOREPLIFETIME(AMainPawn, bMultiTarget);
}

void AMainPawn::OnRep_TransformOnAuthority() {
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
	if (Role < ROLE_Authority) {
		//Alpha = GetWorld()->DeltaTimeSeconds;
		/*
		   if (GetWorld()) {
		   NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
		   lastUpdate = GetWorld()->RealTimeSeconds;
		   }
		 */

		// store starttransform
		TransformOnClient = GetTransform();
		// reset blendfactor
		LerpProgress = 0.0f;

		FVector Direction = LinearVelocity.GetSafeNormal();
		float Velocity = LinearVelocity.Size() * PredictionAmount;

		TargetTransform = FTransform(TransformOnAuthority);
		TargetTransform.AddToTranslation(Direction * Velocity);

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta/*seconds*/, FColor::Red, "Transform received");
	}

}

void AMainPawn::OnRep_LinearVelocity() {
	if (Role < ROLE_Authority) {
		// if (ArmorMesh) ArmorMesh->SetPhysicsLinearVelocity(LinearVelocity, false);
	}
}

void AMainPawn::OnRep_AngularVelocity() {
	if (Role < ROLE_Authority) {
		// if (ArmorMesh) ArmorMesh->SetPhysicsAngularVelocity(AngularVelocity, false);
	}
}

void AMainPawn::CalculateVelocityDeltas() {
	VelForwardDelta = MaxVelocity - DefaultForwardVel;
	VelBackwardsDelta = -MinVelocity + DefaultForwardVel;

	ConstantStrafeAcceleration = FMath::Abs(MaxStrafeVel / TimeToMaxStrafeVel);
}

void AMainPawn::GetPing() {
	if (State) {
		Ping = State->ExactPing * 0.001f;
		return;
	}
	if (GetWorld()->GetFirstPlayerController()) {      // get ping
		State = Cast<APlayerState>(
				GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
		if (State) {
			Ping = State->ExactPing * 0.001f;
			// client has now the most recent ping in seconds
		}
	}

}

// Called to bind functionality to input
void AMainPawn::SetupPlayerInputComponent(class UInputComponent *InputComponent) {
	Super::SetupPlayerInputComponent(InputComponent);

	// action events
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AMainPawn::ZoomIn);
	InputComponent->BindAction("ZoomIn", IE_Released, this, &AMainPawn::ZoomOut);
	InputComponent->BindAction("FreeCamera", IE_Pressed, this, &AMainPawn::ActivateFreeCamera);
	InputComponent->BindAction("FreeCamera", IE_Released, this, &AMainPawn::DeactivateFreeCamera);
	InputComponent->BindAction("Fire Gun Action", IE_Pressed, this, &AMainPawn::StartGunFire);
	InputComponent->BindAction("Fire Gun Action", IE_Released, this, &AMainPawn::StopGunFire);
	InputComponent->BindAction("StopMovement", IE_Pressed, this, &AMainPawn::StopMovement);
	InputComponent->BindAction("Boost", IE_Pressed, this, &AMainPawn::StartBoost);
	InputComponent->BindAction("Boost", IE_Released, this, &AMainPawn::StopBoost);
	InputComponent->BindAction("SwitchTarget", IE_Pressed, this, &AMainPawn::SwitchTargetPressed);
	InputComponent->BindAction("SwitchTarget", IE_Released, this, &AMainPawn::SwitchTargetReleased);

	// axis events
	InputComponent->BindAxis("MoveForward", this, &AMainPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMainPawn::MoveRight);

	InputComponent->BindAxis("LookUp", this, &AMainPawn::PitchCamera);
	InputComponent->BindAxis("LookRight", this, &AMainPawn::YawCamera);

}

//Input functions
void AMainPawn::MoveForward(float AxisValue) {
	if (bBoostPressed) {
		MovementInput.X = 1.0f;
	}
	else {
		MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
	}
}


// axis events
InputComponent->BindAxis("MoveForward", this, &AMainPawn::MoveForward);
InputComponent->BindAxis("MoveRight", this, &AMainPawn::MoveRight);

InputComponent->BindAxis("LookUp", this, &AMainPawn::PitchCamera);
InputComponent->BindAxis("LookRight", this, &AMainPawn::YawCamera);

}

//Input functions
void AMainPawn::MoveForward(float AxisValue) {
	if (bBoostPressed) {
		MovementInput.X = 1.0f;
	}
	else {
		MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
	}
}

void AMainPawn::MoveRight(float AxisValue) {
	MovementInput.Y = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
}

void AMainPawn::PitchCamera(float AxisValue) {
	CameraInput.Y = AxisValue;
}

void AMainPawn::YawCamera(float AxisValue) {
	CameraInput.X = AxisValue;

	//Rotate our actor's yaw, which will turn our camera because we're attached to it
	//{
	//	FRotator NewRotation = GetActorRotation();
	//	NewRotation.Yaw += CameraInput.X;
	//	SetActorRotation(NewRotation);
	//}
}

void AMainPawn::ZoomIn() {
	bZoomingIn = true;
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(1, 0.0f/*seconds*/, FColor::Red, "ZoomPressed");
	Camera->FieldOfView = 30.0f;
}

void AMainPawn::ZoomOut() {
	bZoomingIn = false;
	Camera->FieldOfView = 90.0f;
}

void AMainPawn::ActivateFreeCamera() {
	bFreeCameraActive = true;
	SpringArm->SetRelativeRotation(FRotator(0, 0, SpringArm->GetRelativeTransform().Rotator().Roll), false, nullptr, ETeleportType::None);
	CurrentSpringArmRotation = SpringArm->GetComponentQuat();
	SpringArm->TargetArmLength = 2500.0f;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 10.0f;
}

void AMainPawn::DeactivateFreeCamera() {
	bFreeCameraActive = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->TargetArmLength = SpringArmLength;
	SpringArm->bEnableCameraRotationLag = false;
	SpringArm->SetRelativeRotation(FRotator::ZeroRotator, false, nullptr, ETeleportType::None);
}

void AMainPawn::StartGunFire() {


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



	// player has gunfire button pressed
	bGunFire = true;
	if (bGunReady && bHasAmmo) { // gun is ready to fire
		// make sure no other gunfire timer is activ by clearing it
		GetWorldTimerManager().ClearTimer(GunFireHandle);
		// activate a new gunfire timer
		GetWorldTimerManager().SetTimer(GunFireHandle, this, &AMainPawn::GunFire, FireRateGun, true, 0.0f);
		// debug
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun ON");
	}
	else if (!GetWorldTimerManager().IsTimerActive(GunFireCooldown)) { // gun is not cooling down but could not be fired
		// enable gun
		bGunReady = true;
		// debug
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun COOLDOWN NOT ACTIVATED");
		// try again to fire gun
		if (bHasAmmo) {
			StartGunFire();
		}
		else {
			if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OUT OF AMMO");
		}
	}
	else {
		// gun is cooling down
		// debug
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun STILL COOLING DOWN");
	}
}

void AMainPawn::StopGunFire() {
	// player has gunfire button released
	bGunFire = false;
	// is a gunfire timer active
	if (GetWorldTimerManager().IsTimerActive(GunFireHandle)) {
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
	// debug
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Gun OFF");
}

void AMainPawn::GunCooldownElapsed() {
	// debug
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, "Gun COOLED");
	// gun has cooled down and is again ready to fire
	bGunReady = true;
	// has the user requested fireing reactivate gunfire
	if (bGunFire) {
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, "Gun CONTINUE");
		StartGunFire();
	}

}

void AMainPawn::GunFire() {
	if (GunAmmunitionAmount > 0) {
		// reset the salve counter
		CurrentSalve = 0;
		// the gunfire timer starts a subtimer that fires all the salves
		GetWorldTimerManager().SetTimer(SalveTimerHandle, this, &AMainPawn::FireSalve, SalveIntervall, true, 0.0f);
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, FString::SanitizeFloat(SalveIntervall) + " SalveDelta; " + FString::SanitizeFloat(FireRateGun) + " FireDelta ");
	}
}

void AMainPawn::FireSalve() {
	if (CurrentSalve < NumSalves) {

		for (uint8 shot = 0; shot < NumProjectiles; ++shot) {
			// choose next avaliable gun sockets or start over from the first if last was used
			CurrGunSocketIndex = (CurrGunSocketIndex + 1) % GunSockets.Num();
			// get the tranform of the choosen socket
			const FTransform &CurrentSocketTransform = ArmorMesh->GetSocketTransform(GunSockets[CurrGunSocketIndex]);
			// calculate a direction and apply weaponspread
			const FVector SpawnDirection = FMath::VRandCone(CurrentSocketTransform.GetRotation().GetForwardVector(), WeaponSpreadRadian);
			// spawn/fire projectile

			const FVector &AdditionalVelocity = ArmorMesh->GetPhysicsLinearVelocityAtPoint(CurrentSocketTransform.GetLocation());

			if (TracerIntervall > 0) {
				const FVector TracerOffset = CurrentSocketTransform.GetLocation() + SpawnDirection * FMath::FRandRange(0.0f, (ProjectileVel + AdditionalVelocity.Size()) * GetWorld()->DeltaTimeSeconds);
				// not every projectile has a tracer
				CurrentTracer = (CurrentTracer + 1) % TracerIntervall;
				SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), CurrentTracer == 0, AdditionalVelocity, TracerOffset);
			}
			else { // if tracerintervall was set to 0 there will be tracers
				SpawnProjectile(FTransform(SpawnDirection.Rotation(), CurrentSocketTransform.GetLocation()), false, AdditionalVelocity);
			}
			// decrease ammunition
			--GunAmmunitionAmount;
			if (GunAmmunitionAmount > 0) {
				// debug
				if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, "Bang Ammo left: " + FString::FromInt(GunAmmunitionAmount));
			}
			else {
				bHasAmmo = false;
				if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, "Gun OUT OF AMMO!");
				GetWorldTimerManager().ClearTimer(SalveTimerHandle);
			}

			// add recoil to rootcomponent
			ArmorMesh->AddImpulseAtLocation(SpawnDirection * GunRecoilForce * FMath::FRandRange(0.5f, 1.5f), CurrentSocketTransform.GetLocation());
		}
		++CurrentSalve;
		return;
	}
	// deactivate the salvetimer
	GetWorldTimerManager().ClearTimer(SalveTimerHandle);
}

void AMainPawn::SpawnProjectile_Implementation(const FTransform &SocketTransform, const bool bTracer, const FVector &FireBaseVelocity, const FVector &TracerStartLocation) {
	// method overridden by blueprint to spawn the projectile
}

void AMainPawn::StopMovement() {
	StopPlayerMovement();
}

void AMainPawn::StopPlayerMovement() {
	Server_StopPlayerMovement();
}
bool AMainPawn::Server_StopPlayerMovement_Validate() {
	return true;
}
void AMainPawn::Server_StopPlayerMovement_Implementation() {
	if (bCanReceivePlayerInput) {
		bGunReady = false;
		// make sure there is no pending movement activation
		GetWorldTimerManager().ClearTimer(StartMovementTimerHandle);
		bCanReceivePlayerInput = false;
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, "Movement STOPPED");
	}
	else {
		GetWorldTimerManager().SetTimer(StartMovementTimerHandle, this, &AMainPawn::StartMovementCoolDownElapsed, 1.0f, false);
	}
}

void AMainPawn::StartMovementCoolDownElapsed() {
	bGunReady = true;
	bCanReceivePlayerInput = true;
	RotControlStrength = TimeOfNoControl;
	MovControlStrength = TimeOfNoControl;
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, "Movement STARTED");
}


void AMainPawn::StartBoost() {
	bBoostPressed = true;
}

void AMainPawn::StopBoost() {
	bBoostPressed = false;
}
void AMainPawn::SwitchTargetPressed() {
	CurrLockOnTarget = nullptr;
	bSwitchTargetPressed = true;
	bContinuousLockOn = false;
	bLockOnDelayActiv = false;
	GetWorldTimerManager().ClearTimer(ContinuousLockOnDelay);
}

void AMainPawn::SwitchTargetReleased() {
	bSwitchTargetPressed = false;
}



// sends Playerinput to server
void AMainPawn::GetPlayerInput(FInputsPackage inputData) {

	Server_GetPlayerInput(inputData);

}

bool AMainPawn::Server_GetPlayerInput_Validate(FInputsPackage inputData) {
	return true;
}

//Server receives Input
void AMainPawn::Server_GetPlayerInput_Implementation(FInputsPackage receivedInputData) {
	float NetDelta;
	if (GetWorld()) {
		NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
		lastUpdate = GetWorld()->RealTimeSeconds;
		if (GEngine && DEBUG)
			GEngine->AddOnScreenDebugMessage(-1, 0/*seconds*/, FColor::Green,
					FString::SanitizeFloat(NetDelta) + "    " +
					FString::FromInt(GetVelocity().Size() * 0.036f) + " km/h");
	}

	if (receivedInputData.PacketNo > Ack) {
		Ack = receivedInputData.PacketNo;
		this->InputPackage = receivedInputData;
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Blue, "accepting Packet = " + FString::FromInt(receivedInputData.PacketNo));
	}
	else {
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, "Packet not Accepted");
	}

	LastAcceptedPacket(Ack);
}

void AMainPawn::LastAcceptedPacket(int16 Ack) {
	Client_LastAcceptedPacket(Ack);
}
void AMainPawn::Client_LastAcceptedPacket_Implementation(int16 acceptedPacket) {
	TArray<FInput> pendingInputs = TArray<FInput>();

	//for (FInput &package : InputPackage.InputDataList) {
	//	if (package.PacketNo > acceptedPacket) {
	//		pendingInputs.Add(package);
	//		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, "Entry still pending");
	//	}
	//	else {
	//		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Green, "Entry removed");
	//	}
	//
	//}

	InputPackage.InputDataList = pendingInputs;



	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Green, "Last acceptet Packet = " + FString::FromInt(acceptedPacket));
	this->Ack = acceptedPacket;
}

inline void AMainPawn::GetCursorLocation(FVector2D &CursorLoc) {
	if (GetController()) {
		APlayerController *controller = Cast<APlayerController>(GetController());
		if (controller) {
			controller->GetMousePosition(CursorLoc.X, CursorLoc.Y);
			if (GEngine && DEBUG)
				GEngine->AddOnScreenDebugMessage(-1, 0/*seconds*/, FColor::Red,
						FString::SanitizeFloat(CursorLoc.X) + " " +
						FString::SanitizeFloat(CursorLoc.Y));
		}
	}
}

inline void AMainPawn::GetViewportSizeCenter(FVector2D &ViewPortSize, FVector2D &ViewPortCenter) {
	if (GetWorld()) {
		if (GetWorld()->GetGameViewport()) {
			GetWorld()->GetGameViewport()->GetViewportSize(ViewPortSize);
			ViewPortCenter = ViewPortSize * 0.5f;
		}
	}
}

inline void AMainPawn::GetMouseInput(FVector2D &MouseInput, FVector2D &CursorLoc, FVector2D &ViewPortCenter) {
	{
		MouseInput = (CursorLoc - ViewPortCenter) / ViewPortCenter;
		MouseInput *= MouseInput.GetSafeNormal().GetAbsMax();
		// deadzone (5 pixel)
		if (MouseInput.Size() < (5.0f / ViewPortSize.X)) MouseInput = FVector2D::ZeroVector;
	}
}

void AMainPawn::TargetLock(){
	// execute on locally controlled instance of pawn
	if(bSwitchTargetPressed && !bLockOnDelayActiv){
		CurrLockOnTarget = nullptr;
	}

	if(bLockOnDelayActiv || (CurrLockOnTarget && !bMultiTarget)) return;	

	float DistanceToClosestActor = BIG_NUMBER;
	for (TActorIterator<AActor> currActor(GetWorld()); currActor; ++currActor) {
		//Try InterFaceCasting
		ITarget_Interface* TheInterface = InterfaceCast<ITarget_Interface>(*currActor);

		//Run the Event specific to the actor, if the actor has the interface
		if(TheInterface){
			if (*currActor == this || (currActor->GetOwner() && currActor->GetOwner() == this) || (currActor->GetInstigator() == GetInstigator())) continue;
			const float Distance = currActor->GetDistanceTo(this);
			if (Distance < DistanceToClosestActor) {
				DistanceToClosestActor = Distance;
				if(!CurrLockOnTarget){
					CurrLockOnTarget = *currActor;
				}
				// multiple targets allowed
				if(bMultiTarget) {
					// add current new closest actor	
					MultiTargets.add(*currActor);
					// if maximum number of multitargets already in array, remove the least close actor
					if(MultiTargets.Size() > MaxMultiTargets){
						MultiTargets.remove(0);
					}
				}
			}
		}
	}
	if (CurrLockOnTarget) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, CurrLockOnTarget->GetName() + " : " + FString::SanitizeFloat(DistanceToClosestActor / 100) + " m");
		if(!bContinuousLockOn){
			// activate the delay for continueous LockOn
			GetWorldTimerManager().SetTimer(ContinuousLockOnDelay, this, &AMainPawn::ActivateContinueousLockOn, 0.5f, false);
			bLockOnDelayActiv = true;
		}
	}
}

void AMainPawn::ActivateContinueousLockOn(){
	bContinuousLockOn = true;
	bLockOnDelayActiv = false;
}

void AMainPawn::OnRep_MultiTarget(){
	// Multitarget activated
	// TODO: implemenation
}

void SetTargets(AActor * MainTarget, TArray<AActor*> OtherTargets){
	if(GetNetMode() == NM_Client){
		Server_SetTargets(MainTarget, OtherTargets);
		return;
	}

	CurrLockOnTarget = MainTarget;
	MultiTargets = OtherTargets;
}

void Server_SetTargets_Implementation(AActor * MainTarget, TArray<AActor*> OtherTargets){
	SetTargets(MainTarget, OtherTargets);
}
void Server_SetTargets_Validate(AActor * MainTarget, TArray<AActor*> OtherTargets){
	return true;
}
