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

	Root = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Root"));
	//RootComponent = Root;
	RootComponent = ArmorMesh;
	//ArmorMesh->AttachTo(RootComponent);
	ArmorMesh->SetCollisionObjectType(ECC_Pawn);
	ArmorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArmorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ArmorMesh->SetSimulatePhysics(true);
	ArmorMesh->SetEnableGravity(false);


	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	SpringArm->AttachTo(RootComponent);
	//SpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 50.0f), FRotator(0.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 0.0f;
	SpringArm->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 4.0f;
	SpringArm->CameraLagMaxDistance = 300.0f;


	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	//Camera->AttachTo(RootComponent);
	Camera->AttachTo(SpringArm, USpringArmComponent::SocketName);
	// PostProcessSettings
	Camera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	Camera->PostProcessSettings.LensFlareIntensity = 0.0f;
	Camera->PostProcessSettings.bOverride_AntiAliasingMethod = true;
	Camera->PostProcessSettings.AntiAliasingMethod = EAntiAliasingMethod::AAM_FXAA;
	Camera->PostProcessSettings.bOverride_MotionBlurAmount = true;
	Camera->PostProcessSettings.MotionBlurAmount = 0.1f;





	//Take control of the default Player
	//AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AMainPawn::BeginPlay() {
	Super::BeginPlay();

	lastUpdate = GetWorld()->RealTimeSeconds;
	PrevReceivedTransform = GetTransform();

	// movement
	TurnRate = MaxTurnRate;
	if (Role < ROLE_Authority) {

		// deactivate physics on clients
		ArmorMesh->SetSimulatePhysics(false);
		NumberOfBufferedNetUpdates = FMath::RoundToInt(Smoothing * NetUpdateFrequency);
		NumberOfBufferedNetUpdates = FMath::Max(NumberOfBufferedNetUpdates, 2);
		LerpVelocity = NetUpdateFrequency / NumberOfBufferedNetUpdates;
		PredictionAmount = (NumberOfBufferedNetUpdates + AdditionalUpdatePredictions) / NetUpdateFrequency;
	}

}

// Called every frame
void AMainPawn::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (IsLocallyControlled() && bCanReceivePlayerInput) {

		const bool bUseOrigTurning = true;

		// get mouse position
		GetCursorLocation(CursorLoc);
		// get viewport size/center
		GetViewportSizeCenter(ViewPortSize, ViewPortCenter);
		// the resulting mouse input
		GetMouseInput(MouseInput, CursorLoc, ViewPortCenter);


		if (bUseOrigTurning) {

			//FVector CursorDirInWorld;
			//// Get local player
			//ULocalPlayer* const LP = GetWorld()->GetFirstLocalPlayerFromController();
			//if (LP && LP->ViewportClient)
			//{
			//	// get the projection data
			//	FSceneViewProjectionData ProjectionData;
			//	if (LP->GetProjectionData(LP->ViewportClient->Viewport, eSSP_FULL, /*out*/ ProjectionData))
			//	{
			//		FMatrix const InvViewProjMatrix = ProjectionData.ComputeViewProjectionMatrix().InverseFast();
			//		FVector OutPos;
			//		FSceneView::DeprojectScreenToWorld(CursorLoc, ProjectionData.GetConstrainedViewRect(), InvViewProjMatrix, /*out*/ OutPos, /*out*/ CursorDirInWorld);
			//	}
			//	else {
			//		// something went wrong, interpret input as zero						
			//		CursorDirInWorld = Camera->GetForwardVector();
			//	}
			//}


			FVector2D InputAxis = (CursorLoc - ViewPortCenter) / ViewPortCenter;
			if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat(InputAxis.X) + " x " + FString::SanitizeFloat(InputAxis.Y));
			if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::SanitizeFloat(InputAxis.Size()));

			const float HalfPIPrecision = 0.5f * PI * CenterPrecision;

			MouseInput.X = FMath::Sin(InputAxis.X * HalfPIPrecision);
			MouseInput.Y = FMath::Sin(InputAxis.Y * HalfPIPrecision);
			MouseInput /= FMath::Sin(HalfPIPrecision);
			MouseInput *= MouseInput.GetSafeNormal().GetAbsMax();

			// deadzone (1%)
			if (MouseInput.Size() < 0.01f) MouseInput = FVector2D::ZeroVector;

			PreviousMouseInput = MouseInput;


		}
		else {

			// smooth turning
			{
				float TurnInterpSpeed = 2.0f;
				//InputSize = MouseInput.Size();	

				if (PreviousMouseInput.X * MouseInput.X > 0.0f
					&& MouseInput.X * MouseInput.X < PreviousMouseInput.X * PreviousMouseInput.X) // new x is smaller
				{
					PreviousMouseInput.X = MouseInput.X;
				}
				else {
					PreviousMouseInput.X = FMath::FInterpConstantTo(PreviousMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed);
				}
				if (PreviousMouseInput.Y * MouseInput.Y > 0.0f
					&& MouseInput.Y * MouseInput.Y < PreviousMouseInput.Y * PreviousMouseInput.Y) // new y is smaller
				{
					PreviousMouseInput.Y = MouseInput.Y;
				}
				else {
					PreviousMouseInput.Y = FMath::FInterpConstantTo(PreviousMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed);
				}
				//OldInputSize = OldMouseInput.Size();
			}
		}

		InputPackage.PacketNo++;

		FInput currentInput;
		currentInput.PacketNo = InputPackage.PacketNo;
		currentInput.MouseInput = PreviousMouseInput;
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(PreviousMouseInput.X) + " x " + FString::SanitizeFloat(PreviousMouseInput.Y));
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::SanitizeFloat(PreviousMouseInput.Size()));
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
		if (InputPackage.InputDataList.Num() > 0) {

			const FInput &currentInput = InputPackage.InputDataList.Last();

			PreviousMouseInput = InputPackage.InputDataList.Last().MouseInput;
			MovementInput = InputPackage.InputDataList.Last().MovementInput;

		}
		if (!bCanReceivePlayerInput) {
			PreviousMouseInput.X = 0.f;
			PreviousMouseInput.Y = 0.f;
			MovementInput.X = 0.f;
			MovementInput.Y = 0.f;
		}


		TargetAngularVelocity = GetActorRotation().RotateVector(FVector(0.0f, PreviousMouseInput.Y * TurnRate, PreviousMouseInput.X * TurnRate));
		if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::SanitizeFloat(TargetAngularVelocity.Size()) + " deg/sec");
		
		{
			//Scale movement input axis values by 100 units per second
			MovementInput = MovementInput.GetSafeNormal() * 56000000.0f;
			FVector NewLocation = GetActorLocation();
			NewLocation += (GetActorForwardVector() * MovementInput.X + GetActorRightVector() * MovementInput.Y) * DeltaTime;
			//SetActorLocation(NewLocation);

			FVector Velocity = NewLocation - GetActorLocation();
			TargetLinearVelocity = FVector(Velocity / DeltaTime);
		}



		float AngVInterpSpeed = 5.0f;
		float LinVInterpSpeed = 10000.0f;

		ArmorMesh->SetPhysicsAngularVelocity(
			FMath::VInterpTo(ArmorMesh->GetPhysicsAngularVelocity(), TargetAngularVelocity, DeltaTime,
				AngVInterpSpeed));
		ArmorMesh->SetPhysicsLinearVelocity(
			FMath::VInterpConstantTo(ArmorMesh->GetPhysicsLinearVelocity(), TargetLinearVelocity, DeltaTime,
				LinVInterpSpeed));


		// movement replication
		TransformOnAuthority = GetTransform();
		LinearVelocity = ArmorMesh->GetPhysicsLinearVelocity();
		AngularVelocity = ArmorMesh->GetPhysicsAngularVelocity();
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


		//GetPing();
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Green, "Ping = " + FString::SanitizeFloat(Ping) + " s");
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
	DOREPLIFETIME(AMainPawn, bCanFireGun);
	DOREPLIFETIME(AMainPawn, TransformOnAuthority);
	DOREPLIFETIME(AMainPawn, LinearVelocity);
	DOREPLIFETIME(AMainPawn, AngularVelocity);
	DOREPLIFETIME(AMainPawn, TargetAngularVelocity);
	DOREPLIFETIME(AMainPawn, TargetLinearVelocity);
	DOREPLIFETIME(AMainPawn, bCanReceivePlayerInput);
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

	//Hook up events for "ZoomIn"
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AMainPawn::ZoomIn);
	InputComponent->BindAction("ZoomIn", IE_Released, this, &AMainPawn::ZoomOut);
	InputComponent->BindAction("Fire Gun Action", IE_Pressed, this, &AMainPawn::StartGunFire);
	InputComponent->BindAction("Fire Gun Action", IE_Released, this, &AMainPawn::StopGunFire);
	InputComponent->BindAction("StopMovement", IE_Pressed, this, &AMainPawn::StopMovement);


	//Hook up every-frame handling for our four axes
	InputComponent->BindAxis("MoveForward", this, &AMainPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMainPawn::MoveRight);
	InputComponent->BindAxis("LookUp", this, &AMainPawn::PitchCamera);
	InputComponent->BindAxis("LookRight", this, &AMainPawn::YawCamera);

}

//Input functions
void AMainPawn::MoveForward(float AxisValue) {
	MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
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

void AMainPawn::StartGunFire() {
	bGunFire = true;
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(1, 0, FColor::Green, "Gun ON");
	GunFire();
	GetWorldTimerManager().SetTimer(GunFireHandle, this, &AMainPawn::GunFire, FireRateGun, true);
}

void AMainPawn::StopGunFire() {
	bGunFire = false;
	if (GunFireHandle.IsValid()) {
		GetWorldTimerManager().ClearTimer(GunFireHandle);
	}
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(1, 0, FColor::Red, "Gun OFF");
}

void AMainPawn::GunFire() {
	if (!bCanFireGun) return;
	if (GEngine && DEBUG) GEngine->AddOnScreenDebugMessage(2, 0, FColor::White, "Bang");
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
	bCanReceivePlayerInput = bCanReceivePlayerInput ? false : true;
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

