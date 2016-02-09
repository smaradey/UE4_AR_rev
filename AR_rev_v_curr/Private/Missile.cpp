// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Missile.h"



// Sets default values
AMissile::AMissile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Initialize all base values

	bReplicates = true;                                    // Set the missile to be replicated	
	bAlwaysRelevant = true;

	// Create static mesh component
	MissileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MissileMesh"));


	//MissileMesh->SetCollisionObjectType(ECC_Dynamic);
	//MissileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MissileMesh->SetCollisionProfileName(TEXT("OverlapAll"));

	if (Role == ROLE_Authority) MissileMesh->OnComponentBeginOverlap.AddDynamic(this, &AMissile::MissileMeshOverlap);

	RootComponent = MissileMesh;

	OnDestroyed.AddDynamic(this, &AMissile::MissileDestruction);
	PrimaryActorTick.bCanEverTick = true;                  // enable Tick
}

void AMissile::MissileDestruction() {
	//

}


void AMissile::MissileHit() {
	if (Role == ROLE_Authority) {
		bHit = true;

		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Blue, "Auth: Hit Rep");
		ServerMissileHit();
		SetActorTickEnabled(false);
		if (RootComponent) RootComponent->SetVisibility(false, true);
		if (MissileTrail) MissileTrail->DeactivateSystem();
		if (MissileTrail) MissileTrail->SetVisibility(true);
		SetLifeSpan(10.0f);
	}
}

void AMissile::ServerMissileHit_Implementation() {
	if (Role < ROLE_Authority) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Blue, "client: Hit Rep");

		SetActorTickEnabled(false);
		if (RootComponent) RootComponent->SetVisibility(false, true);
		if (MissileTrail) MissileTrail->DeactivateSystem();
		if (MissileTrail) MissileTrail->SetVisibility(true);

		if (Explosion) {
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, "client: Explosion");
			UParticleSystemComponent* Hit = UGameplayStatics::SpawnEmitterAtLocation(this, Explosion, GetActorLocation(), GetActorRotation(), true);
		}

	}
}


void AMissile::MissileMeshOverlap(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (bHit) return;

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::White, "Overlap Event");
	if (Role == ROLE_Authority) {
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, " Authority: Overlap Event");
		// missileMesh is overlapping with something	
		// Other Actor is the actor that triggered the event. Check that is not ourself.  
		if ((OtherActor) && (OtherActor != this) && (OtherComp)) {
			// do sth
			FString InstigatorOther;
			FString InstigatorThis;

			if (CurrentTarget) {
				if (CurrentTarget->GetOwner() == OtherActor) {
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Green, "Auth: Target HIT");
					CurrentTarget->GetOwner()->ReceiveAnyDamage(100.0f, nullptr, GetInstigatorController(), this);
					MissileHit();  // temp
					return;
				}
			}

			if (OtherActor->GetInstigator()) {
				InstigatorOther = OtherActor->GetInstigator()->GetName();
			}
			else {
				InstigatorOther = "NONE";
			}

			if (GetInstigator()) {
				InstigatorThis = GetInstigator()->GetName();
			}
			else {
				InstigatorThis = "NONE";
			}
			if (OtherActor->GetInstigator() == GetInstigator() && GetInstigator()) {
				if (GEngine) GEngine->AddOnScreenDebugMessage(2, 3.0f/*seconds*/, FColor::Green, "Same Instigator");
				return;
			}
			if (GEngine) GEngine->AddOnScreenDebugMessage(1, 3.0f/*seconds*/, FColor::Red, "Instigator - Other: " + InstigatorOther + "; Missile: " + InstigatorThis);

			// do sth
			FString OwnerOther;
			FString OwnerThis;

			if (OtherComp->GetOwner()) {
				OwnerOther = OtherComp->GetOwner()->GetName();
			}
			else {
				OwnerOther = "NONE";
			}

			if (GetOwner()) {
				OwnerThis = GetOwner()->GetName();
			}
			else {
				OwnerThis = "NONE";
			}
			if (OtherComp->GetOwner() != GetOwner()) {
				if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::White, " Auth: sth. HIT");
				MissileHit();
				return;
			}
			if (GEngine) GEngine->AddOnScreenDebugMessage(4, 3.0f/*seconds*/, FColor::Red, "Owner - Other: " + OwnerOther + "; Missile: " + OwnerThis);
		}
	}
}


// replication of variables
void AMissile::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	DOREPLIFETIME_CONDITION(AMissile, MaxTurnrate, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, MaxVelocity, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AccelerationTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AdvancedMissileMinRange, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, AdvancedMissileMaxRange, COND_InitialOnly);
	DOREPLIFETIME(AMissile, MissileLock);
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

#if WITH_EDITOR
void AMissile::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// do stuff e.g.
	//AdvancedMissileMinRange = MaxVelocity;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

//allows calculation of missing values that have dependencies
void AMissile::PostInitProperties()
{
	Super::PostInitProperties();
	// do stuff e.g.
	// AdvancedMissileMinRange = MaxVelocity;
}

// Called when the game starts or when spawned
void AMissile::BeginPlay()
{
	Super::BeginPlay();

	if (Role == ROLE_Authority && bReplicates) {           // check if current actor has authority
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
		// start a timer that executes a function (multicast)
		FTimerHandle TimerHandle;
		//const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &AMissile::RunsOnAllClients);
		//GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, NetUpdateFrequency, true, 0.0f);
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AMissile::RunsOnAllClients, NetUpdateFrequency, true, 0.0f);

		MaxLifeTime = Range / Velocity;          // calculate max missile liftime (t = s/v)
		InitialLifeSpan = MaxLifeTime + 5.0f;              // set missile lifetime
	}
	Acceleration = 1.0f / AccelerationTime;

	if (MissileMesh && MissileTrailSingle && Role < ROLE_Authority) {
		FVector SpawnLocation;
		if (MissileMesh->DoesSocketExist(FName("booster"))) {
			SpawnLocation = MissileMesh->GetSocketLocation(FName("booster"));
			MissileTrail = UGameplayStatics::SpawnEmitterAttached(MissileTrailSingle, MissileMesh, FName("booster"));
		}
		else {
			SpawnLocation = GetActorLocation();
			MissileTrail = UGameplayStatics::SpawnEmitterAttached(MissileTrailSingle, MissileMesh);
		}

	}
	if (SmokeTrailTick) UParticleSystemComponent* Trail = UGameplayStatics::SpawnEmitterAtLocation(this, SmokeTrailTick, SpawnLocation, GetActorRotation(), true);

}

// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	LifeTime += DeltaTime;                                 // store lifetime

	if (CurrentTarget) {
		CurrentTargetLocation = CurrentTarget->GetComponentLocation();
		DirectionToTarget = CurrentTargetLocation - GetActorLocation();
		DistanceToTarget = DirectionToTarget.Size();

		// actor is authority
		if (Role == ROLE_Authority) {

			float MissileTravelDistance = Velocity * DeltaTime;               // the distance between the current missile location and the next location

																			  // is the target inside explosionradius? (missiletraveldistance is for fast moving missiles with low fps)
			if (DistanceToTarget < TargetDetectionRadius + MissileTravelDistance && bNotFirstTick) {
				// TODO			
				if (CurrentTarget && CurrentTarget->GetOwner()) {
					CurrentTarget->GetOwner()->ReceiveAnyDamage(100.0f, nullptr, GetInstigatorController(), this);
				}
				MissileHit();  // temp
			}
		}
	}


	if (MissileLock)	Homing(DeltaTime);                                     // perform homing to the target by rotating, both clients and server

	// the distance the missile will be moved at the end of the current tick
	MovementVector = GetActorForwardVector() * DeltaTime * Velocity;


	if (Role == ROLE_Authority) {
		// is authority
		if (LifeTime > MaxLifeTime) {                      //  reached max lifetime -> explosion etc.

			//if (ExplosionEffect) ExplosionEffect->Activate(true); // not yet working
			MissileHit();  // temp
		}

		// store current missile transform of client (replicated)
		MissileTransformOnAuthority = GetTransform();

		//if (GEngine) GEngine->AddOnScreenDebugMessage(1, DeltaTime/*seconds*/, FColor::Red, "Authority");

		// store current location for next Tick
		LastActorLocation = GetActorLocation();

	}
	else {
		// is NOT authority

		if (GetWorld()->GetFirstPlayerController()) {      // get ping
			State = Cast<APlayerState>(GetWorld()->GetFirstPlayerController()->PlayerState); // "APlayerState" hardcoded, needs to be changed for main project
			if (State) {
				Ping = float(State->Ping) * 0.001f;
				// debug display ping on screen
				//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, DeltaTime/*seconds*/, FColor::Green, FString::SanitizeFloat(Ping));

				// client has now the most recent ping in seconds
			}
		}
	}

	// is missile is still accelerating? 
	if (!bReachedMaxVelocity) {
		Velocity += Acceleration * DeltaTime * MaxVelocity;          // inrease Velocity
		Turnrate += Acceleration * DeltaTime * MaxTurnrate;          // inrease Turnrate
		// has reached max velocity?
		if (Velocity > MaxVelocity) {
			Velocity = MaxVelocity;
			Turnrate = MaxTurnrate;
			bReachedMaxVelocity = true;                              // has now reached max velocity
		}
		bNotFirstTick = true;
	}
	// perform movement
	AddActorWorldOffset(MovementVector);

	// spawn missiletrail
	if (MissileMesh && SmokeTrailTick && Role < ROLE_Authority) {
		FVector SpawnLocation;
		if (MissileMesh->DoesSocketExist(FName("booster"))) {
			SpawnLocation = MissileMesh->GetSocketLocation(FName("booster"));
		}
		else {
			SpawnLocation = GetActorLocation();
		}
		UParticleSystemComponent* Trail = UGameplayStatics::SpawnEmitterAtLocation(this, SmokeTrailTick, SpawnLocation, GetActorRotation(), true);
	}

}

// perform homing to the target by rotating
void AMissile::Homing(float DeltaTime) {
	if (!CurrentTarget) return;                                           // no homing when there is no valid target



	// is target prediction active?
	if (AdvancedHoming) {
		// target prediction
		TargetVelocity = (CurrentTargetLocation - LastTargetLocation) / DeltaTime;  // A vector with v(x,y,z) = [cm/s]
		LastTargetLocation = CurrentTargetLocation;                                 // store current targetlocation for next recalculation of target velocity

		// calculate the location where missile and target will hit each other
		PredictedTargetLocation = LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVelocity, Velocity);

		// a factor (0.0f - 1.0f) so that the missile is only following the target when far away and is predicting the targetlocation when close
		AdvancedHomingStrength = FMath::GetMappedRangeValueClamped(
			FVector2D(AdvancedMissileMaxRange, AdvancedMissileMinRange),
			FVector2D(0.0f, 1.0f),
			DistanceToTarget);
		//debug
		if (GEngine) GEngine->AddOnScreenDebugMessage(4, DeltaTime/*seconds*/, FColor::White, FString::SanitizeFloat(AdvancedHomingStrength));

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
//----------------------------------------------------- TESTING ------------------------------------------------

// testing
void AMissile::ServerSetFlag()
{
	if (HasAuthority() && !bFlag) // Ensure Role == ROLE_Authority
	{
		bFlag = true;
		OnRep_Flag(); // Run locally since we are the server this won't be called automatically.
	}
}

void AMissile::OnRep_MissileTransformOnAuthority()
{
	// When this is called, bFlag already contains the new value. This
	// just notifies you when it changes.
	if (Role < ROLE_Authority) {

		SetActorTransform(MissileTransformOnAuthority);
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

