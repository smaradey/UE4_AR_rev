// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Missile.h"

// Sets default values
AMissile::AMissile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Initialize all base values
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	NetCullDistanceSquared = 100000.0f * 100000.0f; // Distance of 1km
	NetUpdateFrequency = 5.0f;

	// Create static mesh component
	MissileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));
	MissileMesh->SetCollisionProfileName(TEXT("OverlapAll"));
	RootComponent = MissileMesh;
	if (Role == ROLE_Authority) MissileMesh->OnComponentBeginOverlap.AddDynamic(this, &AMissile::MissileMeshOverlap);

	// A sphere that acts as explosionradius/targetdetection
	ActorDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ActorDetection"));
	ActorDetectionSphere->bAutoActivate = false;
	ActorDetectionSphere->AttachTo(RootComponent);
	ActorDetectionSphere->InitSphereRadius(TargetDetectionRadius);
	ActorDetectionSphere->SetCollisionProfileName(TEXT("Custom"));
	ActorDetectionSphere->SetCanEverAffectNavigation(false);
	//ActorDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMissile::OverlappingATarget);

	// Explosionsoundeffect
	ExplosionSound = CreateDefaultSubobject<UAudioComponent>(TEXT("ExplosionSound"));
	ExplosionSound->bAutoActivate = false;

	// Missileboostersoundeffect
	MissileEngineSound = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineSound"));
	MissileEngineSound->bAutoActivate = true;

	// Missiletrailparticlesystem
	MissileTrail = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MissileTrail"));
	MissileTrail->bAutoActivate = false;

	


	// binding an a function to event OnDestroyed
	OnDestroyed.AddDynamic(this, &AMissile::MissileDestruction);
}

//allows calculation of missing values that have dependencies
void AMissile::PostInitProperties()
{
	Super::PostInitProperties();
	// do stuff e.g.
	// AdvancedMissileMinRange = MaxVelocity;
}

#if WITH_EDITOR
void AMissile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// do stuff e.g.
	//AdvancedMissileMinRange = MaxVelocity;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// replication of variables
void AMissile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME_CONDITION(AMissile, MaxTurnrate, COND_InitialOnly);
	DOREPLIFETIME(AMissile, MaxVelocity);
	DOREPLIFETIME_CONDITION(AMissile, InitialVelocity, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AccelerationTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AdvancedMissileMinRange, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AdvancedMissileMaxRange, COND_InitialOnly);
	DOREPLIFETIME(AMissile, MissileLock);
	DOREPLIFETIME_CONDITION(AMissile, bBombingMode, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, BombingTargetLocation, COND_InitialOnly);	

	DOREPLIFETIME(AMissile, CurrentTarget);
	DOREPLIFETIME_CONDITION(AMissile, AdvancedHoming, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, SpiralHoming, COND_InitialOnly);
	DOREPLIFETIME(AMissile, CustomSpiralOffset);
	DOREPLIFETIME(AMissile, SpiralDirection);
	DOREPLIFETIME(AMissile, SpiralStrength);
	DOREPLIFETIME(AMissile, SpiralVelocity);
	DOREPLIFETIME_CONDITION(AMissile, SpiralDeactivationDistance, COND_InitialOnly);

	DOREPLIFETIME(AMissile, bHit);

	DOREPLIFETIME(AMissile, MissileTransformOnAuthority);
	DOREPLIFETIME(AMissile, IntegerArray);
	DOREPLIFETIME(AMissile, bFlag);
	DOREPLIFETIME(AMissile, bSomeBool);
}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();

	// server behaviour
	if (Role == ROLE_Authority) {
		if (bBombingMode) {
			CurrentTarget = nullptr;
			ActorDetectionSphere->Activate();
			ActorDetectionSphere->SetSphereRadius(TargetDetectionRadius);
		}
		else {
			ActorDetectionSphere->DestroyComponent();
		}

		// create spiraling behaviour
		{
			if (CustomSpiralOffset != 0.0f) {
				CustomSpiralOffset = FMath::FRandRange(0.0f, 360.f);
			}
			if (SpiralDirection != 0) {
				SpiralDirection = FMath::Sign(SpiralDirection);
			}
			else {
				SpiralDirection = (FMath::RandBool()) ? -1.0f : 1.0f;
			}
			if (RandomizeSpiralVelocity) {
				SpiralVelocity *= FMath::FRandRange(0.5f, 1.5f);
			}
		}

		MaxVelocity *= FMath::FRandRange(0.95f, 1.05f);

		// calculate max missile liftime (t = s/v)
		MaxFlightTime = Range / MaxVelocity;
		SetLifeSpan(MaxFlightTime + MissileTrailLifeSpan);              // set missile lifespan
	}

	// clients and authority
	{
		if (bBombingMode) {
			AdvancedHoming = false;
			MissileLock = true;
			CurrentTargetLocation = BombingTargetLocation;
		}
		{
			Velocity = InitialVelocity;
			Acceleration = 1.0f / AccelerationTime;
			FTimerHandle AccerlerationStarter;
			GetWorldTimerManager().SetTimer(AccerlerationStarter, this, &AMissile::EnableAcceleration, 0.2f, false);
		}
	}

	// clients only
	if (Role < ROLE_Authority) {
		NetUpdateInterval = 1.0f / NetUpdateFrequency;
		//ActorDetectionSphere->DestroyComponent();
		//ExplosionSound->AttachTo(RootComponent);
		//if (MissileMesh->DoesSocketExist(FName("booster"))) {
		//	MissileTrail->AttachTo(MissileMesh, FName("booster"));
		//	MissileTrail->Activate();
		//	MissileEngineSound->AttachTo(MissileMesh, FName("booster"));
		//}
	}
	ActorDetectionSphere->DestroyComponent();
	ExplosionSound->AttachTo(RootComponent);
	if (MissileMesh->DoesSocketExist(FName("booster"))) {
		MissileTrail->AttachTo(MissileMesh, FName("booster"));
		MissileTrail->Activate();
		MissileEngineSound->AttachTo(MissileMesh, FName("booster"));
	}
}

// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// count missile lifetime
	LifeTime += DeltaTime;

	// client and server
	if (bBombingMode) {
		DirectionToTarget = CurrentTargetLocation - GetActorLocation();
		DistanceToTarget = DirectionToTarget.Size();
	}
	else {
		if (CurrentTarget) {
			CurrentTargetLocation = CurrentTarget->GetComponentLocation();
			DirectionToTarget = CurrentTargetLocation - GetActorLocation();
			DistanceToTarget = DirectionToTarget.Size();
		}
	}

	// server only
	if (Role == ROLE_Authority) {
		if (GetOwner()) { // not working???
			if (GetOwner()->IsPendingKill()) {
				MissileHit();
				return;
			}
		}

		if (LifeTime > MaxFlightTime) {
			CurrentTarget = nullptr;
			MissileHit();
			return;
		}

		// in bombing mode explode when reaching hominglocation
		if (bBombingMode) {
			if (FVector::DotProduct(DirectionToTarget, GetActorForwardVector()) < 0.0f) {
				MissileHit();
				return;
			}
		}

		// is the target inside explosionradius? (missiletraveldistance is for fast moving missiles with low fps)
		if (DistanceToTarget < TargetDetectionRadius + Velocity * DeltaTime && bNotFirstTick && CurrentTarget) {
			// explode and damage target when target is in range
			{
				bDamageTarget = true;
				HitTarget(CurrentTarget ? ((CurrentTarget->GetOwner()) ? CurrentTarget->GetOwner() : nullptr) : nullptr);
				return;
			}
		}
	}

	// clients and server
	{
		// perform homing to the target by rotating, both clients and server
		if (MissileLock) Homing(DeltaTime);

		// the distance the missile will be moved at the end of the current tick
		MovementVector = GetActorForwardVector() * DeltaTime * Velocity;

		// is missile is still accelerating? 
		if (bCanAccelerate) {
			Turnrate = MaxTurnrate;
			if (!bReachedMaxVelocity) {
				// inrease Velocity
				Velocity += Acceleration * DeltaTime * MaxVelocity;

				//// inrease Turnrate
				//Turnrate += Acceleration * DeltaTime * MaxTurnrate;          

				// has reached max velocity?
				if (Velocity > MaxVelocity) {
					Velocity = MaxVelocity;
					//Turnrate = MaxTurnrate;
					bReachedMaxVelocity = true;
					bCanAccelerate = false;
					// has now reached max velocity
				}
			}
		}
	}

	// server only
	if (Role == ROLE_Authority) {
		// perform movement
		AddActorWorldOffset(MovementVector);
		// current missile transform for replication to client
		MissileTransformOnAuthority = GetTransform();
	}

	// clients only
	if (Role < ROLE_Authority) {
		// perform movement with correction
		if (LocationCorrectionTimeLeft > 0.0f) {
			AddActorWorldOffset(MovementVector + (ClientLocationError * DeltaTime));
			LocationCorrectionTimeLeft -= DeltaTime;
		}
		else {
			AddActorWorldOffset(MovementVector);
		}

		// ping testing
		{
			//if (GetWorld()->GetFirstPlayerController()) {      // get ping
			//	State = Cast<APlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
			//	if (State) {
			//		Ping = float(State->Ping) * 0.001f;
			//		// debug display ping on screen
			//		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Green, FString::SanitizeFloat(Ping));

			//		// client has now the most recent ping in seconds
			//	}
			//}
		}
	}

	// clients and server
	{
		// store current location for next Tick
		LastActorLocation = GetActorLocation();
		bNotFirstTick = true;
	}
}

void AMissile::MissileDestruction() {
	//
}

// called on server for multicast of explosion
void AMissile::MissileHit() {
	if (Role == ROLE_Authority) {
		ServerMissileHit();
		Explode();
	}
}

void AMissile::ServerMissileHit_Implementation() {
	if (Role < ROLE_Authority) {
		Explode();
	}
}

void AMissile::HitTarget_Implementation(class AActor* TargetedActor) {
	if (Role == ROLE_Authority && !bHit) {
		SetLifeSpan(MissileTrailLifeSpan);
		if (bDamageTarget && CurrentTarget) {
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Green, "Auth: Target HIT");
			CurrentTarget->GetOwner()->ReceiveAnyDamage(FMath::RandRange(DamageMin, DamageMax), nullptr, GetInstigatorController(), this);
			bHit = true;
			MissileHit();
		}
	}
}

void AMissile::Explode() {
	if (Explosion/* && Role < ROLE_Authority*/) {

		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, "client: Explosion");
		UParticleSystemComponent* Hit = UGameplayStatics::SpawnEmitterAtLocation(this, Explosion, GetActorLocation(), GetActorRotation(), true);
		if (ExplosionSound) ExplosionSound->Activate();
	}
	if (MissileEngineSound) MissileEngineSound->Deactivate();
	SetActorTickEnabled(false);
	if (ActorDetectionSphere) ActorDetectionSphere->DestroyComponent();
	if (RootComponent) RootComponent->SetVisibility(false, true);
	if (MissileTrail) MissileTrail->DeactivateSystem();
	if (MissileTrail) MissileTrail->SetVisibility(true);
}

void AMissile::OverlappingATarget(class AActor* OtherActor/*, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult*/) {
	if (Role == ROLE_Authority) {
		if (bBombingMode && OtherActor) {
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f/*seconds*/, FColor::Green, "Overlapping in bombing mode");
			if (OtherActor == GetOwner()) return;
			CurrentTarget = OtherActor->GetRootComponent();
			bDamageTarget = true;
			HitTarget(CurrentTarget ? ((CurrentTarget->GetOwner()) ? CurrentTarget->GetOwner() : nullptr) : nullptr);
		}
	}
}

void AMissile::MissileMeshOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	// if missile has already exploded abort function
	if (bHit) return;
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::White, "Overlap Event");

	if (Role == ROLE_Authority) {
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, " Authority: Overlap Event");
		// missileMesh is overlapping with something	
		// Other Actor is the actor that triggered the event. Check that is not the missile.  
		if (OtherActor && (OtherActor != this) && OtherComp) {

			// did the missile hit the target?
			if (CurrentTarget) {
				if (CurrentTarget->GetOwner() == OtherActor) {
					bDamageTarget = true;
					HitTarget(CurrentTarget ? ((CurrentTarget->GetOwner()) ? CurrentTarget->GetOwner() : nullptr) : nullptr);
					return;
				}
			}
			// missile is overlapping with another missile fired by the same player
			if (OtherActor->GetInstigator() == GetInstigator() && GetInstigator()) {
				//if (GEngine) GEngine->AddOnScreenDebugMessage(2, 3.0f/*seconds*/, FColor::Green, "Same Instigator");
				return;
			}
			// did the missile hit something else?
			if (OtherComp->GetOwner() != GetOwner()) {
				//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::White, " Auth: sth. HIT");
				MissileHit();
				return;
			}
		}
	}
}

void AMissile::EnableAcceleration() {
	bCanAccelerate = true;
}

void AMissile::OnRep_MissileTransformOnAuthority()
{
	if (Role < ROLE_Authority) {
		SetActorRotation(MissileTransformOnAuthority.GetRotation());                                                                       // correct Actor rotation
		ClientLocationError = (MissileTransformOnAuthority.GetLocation() - GetActorLocation()) * NetUpdateFrequency;                       // get location error scaled by replication frequency
		LocationCorrectionTimeLeft = NetUpdateInterval;
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, "LocError: " + FString::SanitizeFloat(ClientLocationError.Size()) + "cm");
	}
}

// perform homing to the target by rotating
void AMissile::Homing(float DeltaTime) {
	if (!CurrentTarget && !bBombingMode) return;                                           // no homing when there is no valid target

	// is target prediction active?
	if (AdvancedHoming) {
		TargetVelocity = CurrentTarget->ComponentVelocity;
		//TargetVelocity = (CurrentTargetLocation - LastTargetLocation) / DeltaTime;  // A vector with v(x,y,z) = [cm/s]
		LastTargetLocation = CurrentTargetLocation;                                 // store current targetlocation for next recalculation of target velocity

		// calculate the location where missile and target will hit each other
		PredictedTargetLocation = LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVelocity, Velocity);

		// a factor (0.0f - 1.0f) so that the missile is only following the target when far away and is predicting the targetlocation when close
		AdvancedHomingStrength = FMath::GetMappedRangeValueClamped(
			FVector2D(AdvancedMissileMaxRange, AdvancedMissileMinRange),
			FVector2D(0.0f, 1.0f),
			DistanceToTarget);
		//debug
		//if (GEngine) GEngine->AddOnScreenDebugMessage(4, DeltaTime/*seconds*/, FColor::White, FString::SanitizeFloat(AdvancedHomingStrength));

		// calculate the new forward vector of the missile by taking the distance to the target into consideration 
		PredictedTargetLocation = FMath::Lerp(CurrentTargetLocation, PredictedTargetLocation, FMath::Sqrt(AdvancedHomingStrength));
		// (sqrt of homing strength so that the transition is not linear)
		//DirectionToTarget = (CurrentTargetLocation + ((PredictedTargetLocation - CurrentTargetLocation) * FMath::Sqrt(AdvancedHomingStrength))) - GetActorLocation();
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Green, "advanced Homing");

		DirectionToTarget = PredictedTargetLocation - GetActorLocation();

		if (SpiralHoming && DistanceToTarget > SpiralDeactivationDistance) {
			float Amplitude = DistanceToTarget * SpiralStrength;

			HomingLocation = PredictedTargetLocation + (Amplitude * FRotationMatrix(DirectionToTarget.Rotation()).GetScaledAxis(EAxis::Y)).RotateAngleAxis((int(SpiralVelocity * LifeTime) % int(360)) * SpiralDirection + CustomSpiralOffset, DirectionToTarget.GetSafeNormal());
			DirectionToTarget = HomingLocation - GetActorLocation();

		}
	}
	else {
		if (SpiralHoming && DistanceToTarget > SpiralDeactivationDistance) {
			float Amplitude = DistanceToTarget * SpiralStrength;
			HomingLocation = CurrentTargetLocation + (Amplitude * FRotationMatrix(DirectionToTarget.Rotation()).GetScaledAxis(EAxis::Y)).RotateAngleAxis((int(SpiralVelocity * LifeTime) % int(360)) * SpiralDirection + CustomSpiralOffset, DirectionToTarget.GetSafeNormal());
			DirectionToTarget = HomingLocation - GetActorLocation();
		}
	}

	DirectionToTarget.Normalize();                            // normalize the direction vector

	// calculate the angle the missile will turn (limited by the max turnspeed [deg/s] )
	AngleToTarget = FMath::Clamp(FMath::RadiansToDegrees(FMath::Acos(DirectionToTarget | GetActorForwardVector())), 0.0f, Turnrate * DeltaTime);
	// debug
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::White, "Turnrate [deg/s] = " + FString::FromInt(AngleToTarget / DeltaTime));

	// rotation axis for turning the missile towards the target
	RotationAxisForTurningToTarget = GetActorForwardVector() ^ DirectionToTarget;

	// rotate the missile forward vector towards target direction
	NewDirection = GetActorForwardVector().RotateAngleAxis(AngleToTarget, RotationAxisForTurningToTarget.GetSafeNormal());

	// apply the new direction as rotation to the missile
	SetActorRotation(NewDirection.Rotation());
}

//returns a location at which has to be aimed in order to hit the target
FVector AMissile::LinearTargetPrediction(
	const FVector &TargetLocation,
	const FVector &StartLocation,
	const FVector &TargetVelocity, // cm/s
	const float ProjectileVelocity) // cm/s	
{
	FVector AB = TargetLocation - StartLocation;
	AB.Normalize();
	FVector vi = TargetVelocity - (FVector::DotProduct(AB, TargetVelocity) * AB);
	return StartLocation + vi + AB * FMath::Sqrt(ProjectileVelocity * ProjectileVelocity - FMath::Pow((vi.Size()), 2.f));
}


//----------------------------------------------------- TESTING ------------------------------------------------

// testing
// replication of the timercalled funtion
void AMissile::RunsOnAllClients() {
	if (Role == ROLE_Authority)
	{
		ServerRunsOnAllClients();
	}
}

// multicasted function
void AMissile::ServerRunsOnAllClients_Implementation() {
	if (Role < ROLE_Authority) {

		//SetActorTransform(MissileTransformOnAuthority);
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f/*seconds*/, FColor::Red, "corrected missile transform");

		//if (GEngine) GEngine->AddOnScreenDebugMessage(2, 3.0f/*seconds*/, FColor::Green, "(Multicast)");
		// currentTime.UtcNow().ToString()
		int64 Milliseconds = currentTime.ToUnixTimestamp() * 1000 + currentTime.GetMillisecond();

		//if (GEngine) GEngine->AddOnScreenDebugMessage(3, 3.0f/*seconds*/, FColor::Green, FString::SanitizeFloat((float)Milliseconds));
	}
}
void AMissile::ServerSetFlag()
{
	if (HasAuthority() && !bFlag) // Ensure Role == ROLE_Authority
	{
		bFlag = true;
		OnRep_Flag(); // Run locally since we are the server this won't be called automatically.
	}
}


// testing
void AMissile::OnRep_Flag()
{
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
}

// testing
void AMissile::Server_RunsOnServer_Implementation()
{
	// Do something here that modifies game state.
}

// testing
bool AMissile::Server_RunsOnServer_Validate()
{
	// Optionally validate the request and return false if the function should not be run.
	return true;
}

// testing
void AMissile::Dealing() {
	if (Role == ROLE_Authority)
	{
		ServerDealing();
	}
}

// testing
void AMissile::ServerDealing_Implementation() {
	//
}

float AMissile::DistanceLineLine(const FVector& a1,
	const FVector& a2,
	const FVector& b1,
	const FVector& b2) {
	return ((a1 - a2)*(b1 ^ b2)).Size() / (b1 ^ b2).Size();
}

//returns the Distance or -1 / WIP
bool AMissile::ClosestPointsOnTwoLines(const FVector& LineStartA,
	const FVector& LineEndA,
	const FVector& LineStartB,
	const FVector& LineEndB,
	FVector& PointA,
	FVector& PointB) {
	const FVector u = LineEndA - LineStartA;
	const FVector v = LineEndB - LineStartB;

	const float X1 = (LineStartA - LineStartB) | u;
	const float X2 = (LineStartA - LineStartB) | v;

	const float Y1 = u | u;
	const float Y2 = v | u;

	const float Z1 = Y2;
	const float Z2 = v | v;

	//X1 + t * Y1 - s * Z1 = 0;
	//X2 + t * Y2 - s * Z2 = 0;

	//(Y2/Y1 * X1 - X2) - s * (Y2 / Y1 * Z1 - Z2) = 0
	float s = (Y2 / Y1 * X1 - X2) / (Y2 / Y1 * Z1 - Z2);
	float t = (-X1 + s * Z1) / Y1;

	PointA = LineStartA + t * u;
	PointB = LineStartB + s * v;

	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, GetWorld()->GetDeltaSeconds()/*seconds*/, FColor::Red, "s = " + FString::SanitizeFloat(s) + " t = " + FString::SanitizeFloat(t));

	return (t*t < 1.0f && s*s < 1.0f);
}




////// example for function replication------------------------
void AMissile::SetSomeBool(bool bNewSomeBool)
{
	//if (GEngine) GEngine->AddOnScreenDebugMessage(4, 20.0f/*seconds*/, FColor::White, FString("Client calls serverfunction to set a bool ").Append(FString(bSomeBool ? "True" : "False")));
	ServerSetSomeBool(bNewSomeBool);
}

bool AMissile::ServerSetSomeBool_Validate(bool bNewSomeBool)
{
	return true;
}

void AMissile::ServerSetSomeBool_Implementation(bool bNewSomeBool)
{
	bSomeBool = bNewSomeBool;
	//if (GEngine) GEngine->AddOnScreenDebugMessage(5, 20.0f/*seconds*/, FColor::Blue, "bool was set on server");
	//if (GEngine) GEngine->AddOnScreenDebugMessage(6, 20.0f/*seconds*/, FColor::Blue, FString(bSomeBool ? "true" : "false"));

}
////// end: example for function replication------------------------

////// example for function replication

void AMissile::RunsOnOwningClientOnly() {
	if (Role == ROLE_Authority)
	{
		Client_RunsOnOwningClientOnly();
	}
}
void AMissile::Client_RunsOnOwningClientOnly_Implementation()
{
	// Do something here to affect the client. This method was called by the server ONLY.
}

////// end: example for function replication


/*GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("This is an on screen message!"));
UE_LOG(LogTemp, Log, TEXT("Log text %f"), 0.1f);
UE_LOG(LogTemp, Warning, TEXT("Log warning"));
UE_LOG(LogTemp, Error, TEXT("Log error"));
FError::Throwf(TEXT("Log error"));
FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Dialog message")));
UE_LOG(LogTemp, Warning, TEXT("Your message"));*/

