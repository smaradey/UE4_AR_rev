// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "MainPawn.h"


// Sets default values
AMainPawn::AMainPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;                                   
	bAlwaysRelevant = true;

	//Create components
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	OurCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	OurCameraSpringArm->AttachTo(RootComponent);
	OurCameraSpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 0.0f), FRotator(0.0f, 0.0f, 0.0f));
	OurCameraSpringArm->TargetArmLength = 0.0f;
	OurCameraSpringArm->SocketOffset = FVector(0.0f,0.0f,75.0f);
	OurCameraSpringArm->bEnableCameraLag = true;
	OurCameraSpringArm->CameraLagSpeed = 4.0f;
	OurCameraSpringArm->CameraLagMaxDistance = 300.0f;
	OurCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GameCamera"));
	OurCamera->AttachTo(OurCameraSpringArm, USpringArmComponent::SocketName);
	OurCamera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	OurCamera->PostProcessSettings.LensFlareIntensity = 0.0f;
	
	

	// Create static mesh component
	ArmorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmorMesh"));
	ArmorMesh->AttachTo(RootComponent);

	//Take control of the default Player
	//AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AMainPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMainPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	
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
			MovementInput = MovementInput.GetSafeNormal() * 1000.0f;
			FVector NewLocation = GetActorLocation();
			NewLocation += GetActorForwardVector() * MovementInput.X * DeltaTime;
			NewLocation += GetActorRightVector() * MovementInput.Y * DeltaTime;
			SetActorLocation(NewLocation);
		}
	}
}

// replication of variables
void AMainPawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME(AMainPawn, bCanFireGun);
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
	{
		FRotator NewRotation = GetActorRotation();
		NewRotation.Yaw += CameraInput.X;
		SetActorRotation(NewRotation);
	}
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
