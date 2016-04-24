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
    //ArmorMesh->AttachTo(RootComponent);
    ArmorMesh->SetCollisionObjectType(ECC_Pawn);
    ArmorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ArmorMesh->SetCollisionProfileName(TEXT("BlockAll"));

    ArmorMesh->SetSimulatePhysics(true);
    ArmorMesh->SetEnableGravity(false);
    
	
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
    SpringArm->AttachTo(RootComponent);
    SpringArm->SetRelativeLocationAndRotation(FVector(-300.0f, 0.0f, 50.0f), FRotator(0.0f, 0.0f, 0.0f));
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

    // movement
    TurnRate = MaxTurnRate;
	if (Role < ROLE_Authority) {
		lastUpdate = GetWorld()->RealTimeSeconds;
		PrevReceivedTransform = GetTransform();
		ArmorMesh->SetSimulatePhysics(false);
	}

}

// Called every frame
void AMainPawn::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    if (IsLocallyControlled() && Role == ROLE_Authority) {
        // get mouse position
        GetCursorLocation(CursorLoc);
        // get viewport size/center
        GetViewportSizeCenter(ViewPortSize, ViewPortCenter);
        // the resulting mouse input
        GetMouseInput(MouseInput, CursorLoc, ViewPortCenter);

        // smooth turning
        {
            float TurnInterpSpeed = 2.0f;
            //InputSize = MouseInput.Size();
            bool q1 = OldMouseInput.X < 0.0f && MouseInput.X < 0.0f && MouseInput.X - OldMouseInput.X > 0.0f;
            bool q2 = OldMouseInput.X > 0.0f && MouseInput.X > 0.0f && MouseInput.X - OldMouseInput.X < 0.0f;
            bool q3 = OldMouseInput.Y < 0.0f && MouseInput.Y < 0.0f && MouseInput.Y - OldMouseInput.Y > 0.0f;
            bool q4 = OldMouseInput.Y > 0.0f && MouseInput.Y > 0.0f && MouseInput.Y - OldMouseInput.Y < 0.0f;

            if (q1 || q2) {
                OldMouseInput.X = MouseInput.X;
            }
            else {
                OldMouseInput.X = FMath::FInterpConstantTo(OldMouseInput.X, MouseInput.X, DeltaTime, TurnInterpSpeed);
            }
            if (q3 || q4) {
                OldMouseInput.Y = MouseInput.Y;
            }
            else {
                OldMouseInput.Y = FMath::FInterpConstantTo(OldMouseInput.Y, MouseInput.Y, DeltaTime, TurnInterpSpeed);
            }
            //OldInputSize = OldMouseInput.Size();
        }

        TargetAngularVelocity = GetActorRotation().RotateVector(FVector(0.0f, OldMouseInput.Y * TurnRate, OldMouseInput.X * TurnRate));

        {
            //Scale movement input axis values by 100 units per second
            MovementInput = MovementInput.GetSafeNormal() * 56000000.0f;
            FVector NewLocation = GetActorLocation();
            NewLocation += (GetActorForwardVector() * MovementInput.X + GetActorRightVector() * MovementInput.Y) * DeltaTime;
            //SetActorLocation(NewLocation);

            FVector Velocity = NewLocation - GetActorLocation();
            TargetLinearVelocity = FVector(Velocity / DeltaTime);
        }

        GetPlayerInput(DeltaTime, MouseInput, MovementInput);

		float AngVInterpSpeed = 360.0f;
		float LinVInterpSpeed = 10000.0f;

		ArmorMesh->SetPhysicsAngularVelocity(
			FMath::VInterpConstantTo(ArmorMesh->GetPhysicsAngularVelocity(), TargetAngularVelocity, DeltaTime,
				AngVInterpSpeed));
		ArmorMesh->SetPhysicsLinearVelocity(
			FMath::VInterpConstantTo(ArmorMesh->GetPhysicsLinearVelocity(), TargetLinearVelocity, DeltaTime,
				LinVInterpSpeed));
    } else if (Role == ROLE_Authority) {
		float AngVInterpSpeed = 10.f;
		float LinVInterpSpeed = 100.f;

		ArmorMesh->SetPhysicsAngularVelocity(
			FMath::VInterpConstantTo(ArmorMesh->GetPhysicsAngularVelocity(), FVector::ZeroVector, DeltaTime,
				AngVInterpSpeed));
		ArmorMesh->SetPhysicsLinearVelocity(
			FMath::VInterpConstantTo(ArmorMesh->GetPhysicsLinearVelocity(), FVector::ZeroVector, DeltaTime,
				LinVInterpSpeed));


        TransformOnAuthority = GetTransform();
       AngularVelocity = ArmorMesh->GetPhysicsAngularVelocity();
       LinearVelocity = ArmorMesh->GetPhysicsLinearVelocity();
    } else {
		FTransform NewTransform;

		TransformBlend += DeltaTime;
		//NewTransform.Blend(TransformOnClient, TransformOnAuthority, TransformBlend / NetDelta);

		//NewTransform.Blend(TransformOnClient, TransformOnAuthority, /*FMath::Min(1.0f,*/(TransformBlend / NetDelta) * 0.8f);
		NewTransform.Blend(TransformOnClient, TransformOnAuthority, FMath::Min(1.f,TransformBlend * NetUpdateFrequency*Smoothing));
		//TransformBlend += DeltaTime;
        SetActorTransform(NewTransform,false,nullptr,ETeleportType::TeleportPhysics);
	}





    //GetPing();
    //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, GetWorld()->DeltaTimeSeconds, FColor::Green, "Ping = " + FString::SanitizeFloat(Ping) + " s");
    //FTransform newTransform;

    //newTransform.Blend(TransformOnClient, TransformOnAuthority, FMath::Min(Alpha * NetUpdateFrequency*0.9f, 1.0f)); //?
    //Alpha += DeltaTime;
    //SetActorTransform(newTransform);



}

// replication of variables
void AMainPawn::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> &OutLifetimeProps) const {
    DOREPLIFETIME(AMainPawn, bCanFireGun);
    DOREPLIFETIME(AMainPawn, TransformOnAuthority);
    DOREPLIFETIME(AMainPawn, LinearVelocity);
    DOREPLIFETIME(AMainPawn, AngularVelocity);
    DOREPLIFETIME(AMainPawn, TargetAngularVelocity);
    DOREPLIFETIME(AMainPawn, TargetLinearVelocity);
}

void AMainPawn::OnRep_TransformOnAuthority() {
    // When this is called, bFlag already contains the new value. This
    // just notifies you when it changes.
    if (Role < ROLE_Authority) {
        //Alpha = GetWorld()->DeltaTimeSeconds;
		
		if (GetWorld()) {
			NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
			lastUpdate = GetWorld()->RealTimeSeconds;
		}
		//TransformOnClient = PrevReceivedTransform;
		TransformOnClient = GetTransform();
		//PrevReceivedTransform = TransformOnAuthority;
		TransformBlend = 0.0f;


		//LinVelServer = ((TransformOnAuthority.GetLocation() - PrevLocationOnServer) / NetDelta).Size();
		//PrevLocationOnServer = TransformOnAuthority.GetLocation();
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, NetDelta/*seconds*/, FColor::Red, "Transform received");


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
    if (GEngine) GEngine->AddOnScreenDebugMessage(1, 3.0f/*seconds*/, FColor::Red, "ZoomPressed");
    Camera->FieldOfView = 30.0f;
}

void AMainPawn::ZoomOut() {
    bZoomingIn = false;
    Camera->FieldOfView = 90.0f;
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

// sends Playerinput to server
void AMainPawn::GetPlayerInput(float DeltaTime, FVector2D CameraInput, FVector2D MovementInput) {
    Server_GetPlayerInput(DeltaTime, CameraInput, MovementInput);
}

bool AMainPawn::Server_GetPlayerInput_Validate(float DeltaTime, FVector2D CameraInput, FVector2D MovementInput) {
    return true;
}

//Server receives Input
void AMainPawn::Server_GetPlayerInput_Implementation(float DeltaTime, FVector2D CameraInput, FVector2D MovementInput) {
    float NetDelta;
    if (GetWorld()) {
        NetDelta = GetWorld()->RealTimeSeconds - lastUpdate;
        lastUpdate = GetWorld()->RealTimeSeconds;
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Green,
                                             FString::SanitizeFloat(NetDelta) + "    " +
                                             FString::FromInt(GetVelocity().Size() * 0.036f) + " km/h");
    }
}

inline void AMainPawn::GetCursorLocation(FVector2D &CursorLoc) {
    if (GetController()) {
        APlayerController *controller = Cast<APlayerController>(GetController());
        if (controller) {
            controller->GetMousePosition(CursorLoc.X, CursorLoc.Y);
            if (GEngine)
                GEngine->AddOnScreenDebugMessage(3, 3.0f/*seconds*/, FColor::Red,
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

