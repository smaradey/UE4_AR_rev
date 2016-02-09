// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "MainPawn.h"


// Sets default values
AMainPawn::AMainPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = true;

	//SetActorEnableCollision(true);

	//Create components
	//RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create static mesh component
	ArmorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmorMesh"));
	//ArmorMesh->AttachTo(RootComponent);
	//ArmorMesh->SetCollisionObjectType(ECC_Pawn);
	//ArmorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArmorMesh->SetCollisionProfileName(TEXT("BlockAll"));

	ArmorMesh->SetSimulatePhysics(true);
	ArmorMesh->SetEnableGravity(false);
	RootComponent = ArmorMesh;

	OurCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	OurCameraSpringArm->AttachTo(RootComponent);
	OurCameraSpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	OurCameraSpringArm->TargetArmLength = 0.0f;
	OurCameraSpringArm->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	OurCameraSpringArm->bEnableCameraLag = true;
	OurCameraSpringArm->CameraLagSpeed = 4.0f;
	OurCameraSpringArm->CameraLagMaxDistance = 300.0f;

	OurCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	OurCamera->AttachTo(OurCameraSpringArm, USpringArmComponent::SocketName);
	OurCamera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	OurCamera->PostProcessSettings.LensFlareIntensity = 0.0f;




	//Take control of the default Player
	//AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AMainPawn::BeginPlay()
{
	Super::BeginPlay();

	// movement
	TurnRate = MaxTurnRate;

}

// Called every frame
void AMainPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	
	/*FVector LocArmor = ArmorMesh->GetComponentLocation();
	FVector DirToRoot = (GetActorLocation() - LocArmor);

	ArmorMesh->AddForce(DirToRoot.GetSafeNormal(), NAME_None, true);*/



	//ArmorMesh->ResetRelativeTransform();
	//RelativeArmorTransform.Blend(ArmorMesh->GetComponentTransform(),GetTransform(), 0.5f); //?
	//ArmorMesh->SetWorldTransform(RootComponent->GetComponentTransform());
	//RelativeArmorTransform = ArmorMesh->GetComponentTransform();



	if (IsLocallyControlled()) {
		ArmorMesh->SetPhysicsAngularVelocity(FMath::VInterpConstantTo(ArmorMesh->GetPhysicsAngularVelocity(), FVector::ZeroVector, DeltaTime, 1000.0f));
		ArmorMesh->SetPhysicsLinearVelocity(FMath::VInterpConstantTo(ArmorMesh->GetPhysicsLinearVelocity(), FVector::ZeroVector, DeltaTime, 1000.0f));

		FVector CurrentPhVel = ArmorMesh->GetPhysicsLinearVelocity();
		// get mouse position
		{
			if (GetController()) {
				if (GetController()->CastToPlayerController()) {
					if (GetController()->CastToPlayerController()->GetMousePosition(CursorLoc.X, CursorLoc.Y)) {
						if (GEngine) GEngine->AddOnScreenDebugMessage(3, 3.0f/*seconds*/, FColor::Red, FString::SanitizeFloat(CursorLoc.X) + " " + FString::SanitizeFloat(CursorLoc.Y));
					}
				}
			}
		}
		// get viewport size/center
		{
			if (GetWorld()) {
				if (GetWorld()->GetGameViewport()) {
					GetWorld()->GetGameViewport()->GetViewportSize(ViewPortSize);
					ViewPortCenter = ViewPortSize * 0.5f;
				}
			}
		}
		// the resulting mouse input
		{
			MouseInput = (CursorLoc - ViewPortCenter) / ViewPortCenter;
			MouseInput *= MouseInput.GetSafeNormal().GetAbsMax();
			// deadzone (5 pixel)
			if (MouseInput.Size() < (5.0f / ViewPortSize.X)) MouseInput = FVector2D::ZeroVector;
		}


		// smooth turning
		{
			//InputSize = MouseInput.Size();
			bool q1 = OldMouseInput.X < 0.0f && MouseInput.X < 0.0f && MouseInput.X - OldMouseInput.X > 0.0f;
			bool q2 = OldMouseInput.X > 0.0f && MouseInput.X > 0.0f && MouseInput.X - OldMouseInput.X < 0.0f;
			bool q3 = OldMouseInput.Y < 0.0f && MouseInput.Y < 0.0f && MouseInput.Y - OldMouseInput.Y > 0.0f;
			bool q4 = OldMouseInput.Y > 0.0f && MouseInput.Y > 0.0f && MouseInput.Y - OldMouseInput.Y < 0.0f;

			if (q1 || q2) {
				OldMouseInput.X = MouseInput.X;
			}
			else {
				OldMouseInput.X = FMath::FInterpConstantTo(OldMouseInput.X, MouseInput.X, DeltaTime, 2.0f);
			}
			if (q3 || q4) {
				OldMouseInput.Y = MouseInput.Y;
			}
			else {
				OldMouseInput.Y = FMath::FInterpConstantTo(OldMouseInput.Y, MouseInput.Y, DeltaTime, 2.0f);
			}
			//OldInputSize = OldMouseInput.Size();
		}
		if (GEngine) GEngine->AddOnScreenDebugMessage(4, 3.0f/*seconds*/, FColor::Green, FString::SanitizeFloat(MouseInput.X) + " " + FString::SanitizeFloat(MouseInput.Y) + " " + FString::SanitizeFloat(OldMouseInput.Size()*TurnRate));
		AddActorLocalRotation(FRotator(OldMouseInput.Y * -TurnRate * DeltaTime, OldMouseInput.X * TurnRate * DeltaTime, 0.0f), false, nullptr);

		////Rotate our actor's yaw, which will turn our camera because we're attached to it
		//{
		//	FRotator NewRotation = GetActorRotation();
		//	NewRotation.Yaw += CameraInput.X;
		//	SetActorRotation(NewRotation);
		//}

		//Rotate our camera's pitch, but limit it so we're always looking downward
		/*
		{
			FRotator NewRotation = OurCameraSpringArm->GetComponentRotation();
			NewRotation.Pitch = FMath::Clamp(NewRotation.Pitch + CameraInput.Y, -80.0f, -15.0f);
			OurCameraSpringArm->SetWorldRotation(NewRotation);
		}*/

		//Handle movement based on our "MoveX" and "MoveY" axes
		{
			if (!MovementInput.IsZero())
			{
				//Scale our movement input axis values by 100 units per second
				MovementInput = MovementInput.GetSafeNormal() * 10000.0f;
				FVector NewLocation = GetActorLocation();
				NewLocation += GetActorForwardVector() * MovementInput.X * DeltaTime;
				NewLocation += GetActorRightVector() * MovementInput.Y * DeltaTime;
				//SetActorLocation(NewLocation);
				FVector Vel = NewLocation - GetActorLocation();
				ArmorMesh->SetPhysicsLinearVelocity(CurrentPhVel + Vel / DeltaTime);




			}
		}
	}
	if (Role == ROLE_Authority) {
		TransformOnAuthority = GetTransform();
		AngularVelocity = ArmorMesh->GetPhysicsAngularVelocity();
		LinearVelocity = ArmorMesh->GetPhysicsLinearVelocity();
	}
	else {
		//GetPing();
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Green, "Ping = " + FString::SanitizeFloat(Ping) + " s");
		//FTransform newTransform;

		//newTransform.Blend(TransformOnClient, TransformOnAuthority, FMath::Min(Alpha * NetUpdateFrequency*0.9f, 1.0f)); //?
		//Alpha += DeltaTime;
		//SetActorTransform(newTransform);
	}


}

// replication of variables
void AMainPawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(AMainPawn, bCanFireGun);
	DOREPLIFETIME(AMainPawn, TransformOnAuthority);
	DOREPLIFETIME(AMainPawn, LinearVelocity);
	DOREPLIFETIME(AMainPawn, AngularVelocity);
}

void AMainPawn::OnRep_TransformOnAuthority()
{
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
	if (Role < ROLE_Authority) {
		//Alpha = GetWorld()->DeltaTimeSeconds;
		//TransformOnClient = GetTransform();

		SetActorTransform(TransformOnAuthority);

		//SetActorTransform(MissileTransformOnAuthority);
	}
}

void AMainPawn::OnRep_LinearVelocity()
{
	if (Role < ROLE_Authority ) {
		if(ArmorMesh) ArmorMesh->SetPhysicsLinearVelocity(LinearVelocity, false);
	}
}

void AMainPawn::OnRep_AngularVelocity()
{
	if (Role < ROLE_Authority) {
		if (ArmorMesh) ArmorMesh->SetPhysicsAngularVelocity(AngularVelocity, false);
	}
}

void AMainPawn::GetPing() {
	if (State) {
		Ping = State->ExactPing * 0.001f;
		return;
	}
	if (GetWorld()->GetFirstPlayerController()) {      // get ping
		State = Cast<APlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
		if (State) {
			Ping = State->ExactPing * 0.001f;
			// client has now the most recent ping in seconds
		}
	}

}

// Called to bind functionality to input
void AMainPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	//Hook up events for "ZoomIn"
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AMainPawn::ZoomIn);
	InputComponent->BindAction("ZoomIn", IE_Released, this, &AMainPawn::ZoomOut);
	InputComponent->BindAction("Fire Gun Action", IE_Pressed, this, &AMainPawn::StartGunFire);
	InputComponent->BindAction("Fire Gun Action", IE_Released, this, &AMainPawn::StopGunFire);


	//Hook up every-frame handling for our four axes
	InputComponent->BindAxis("MoveForward", this, &AMainPawn::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AMainPawn::MoveRight);
	InputComponent->BindAxis("LookUp", this, &AMainPawn::PitchCamera);
	InputComponent->BindAxis("LookRight", this, &AMainPawn::YawCamera);

}

//Input functions
void AMainPawn::MoveForward(float AxisValue)
{
	MovementInput.X = FMath::Clamp<float>(AxisValue, -1.0f, 1.0f);
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
	if (GEngine) GEngine->AddOnScreenDebugMessage(1, 3.0f/*seconds*/, FColor::Red, "ZoomPressed");
	OurCamera->FieldOfView = 30.0f;
}

void AMainPawn::ZoomOut()
{
	bZoomingIn = false;
	OurCamera->FieldOfView = 90.0f;
}

void AMainPawn::StartGunFire() {
	bGunFire = true;
	if (GEngine) GEngine->AddOnScreenDebugMessage(1, GetWorld()->DeltaTimeSeconds, FColor::Green, "Gun ON");
	GunFire();
	GetWorldTimerManager().SetTimer(GunFireHandle, this, &AMainPawn::GunFire, FireRateGun, true);
}
void AMainPawn::StopGunFire() {
	bGunFire = false;
	if (GunFireHandle.IsValid()) {
		GetWorldTimerManager().ClearTimer(GunFireHandle);
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(1, GetWorld()->DeltaTimeSeconds, FColor::Red, "Gun OFF");
}

void AMainPawn::GunFire() {
	if (!bCanFireGun) return;
	if (GEngine) GEngine->AddOnScreenDebugMessage(2, GetWorld()->DeltaTimeSeconds, FColor::White, "Bang");
}
