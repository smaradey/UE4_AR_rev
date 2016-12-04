// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Missile.h"

// custom collision profile created in Editor, Tracechannel found in DefaultEngine.ini
//#define ECC_Missile ECC_GameTraceChannel2

void AMissile::OnConstruction(const FTransform& Transform)
{
	LOG("Missile: OnConstruction")
		mRemainingBoostDistance = mProperties.MaxRange;

	if (ActorDetectionSphere)ActorDetectionSphere->SetSphereRadius(mProperties.ExplosionRadius);
	// Attach
	LOGA("Missile: BeginPlay: BoosterSocket = \"%s\"", *BoosterSocket.ToString())
		if (Mesh && Mesh->DoesSocketExist(BoosterSocket))
		{
			if (mMissileTrail)
			{
				mMissileTrail->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, BoosterSocket);
				mMissileTrail->Activate();
			}
			if (mBoosterSound) mBoosterSound->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform, BoosterSocket);
		}
		else
		{
			if (mMissileTrail)
			{
				mMissileTrail->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform);
				mMissileTrail->Activate();
			}
			if (mBoosterSound)mBoosterSound->AttachToComponent(Mesh, FAttachmentTransformRules::KeepRelativeTransform);
		}
}

void AMissile::Explode_Implementation(UObject* object, const float Delay)
{
	if (object)
	{
		LOGA("Missile: Received mExplosion Command from \"%s\"", *object->GetName())
	}
	if (bDetonated) return;

	if(Delay > 0.0f){
		FTimerManager& TimerManager = GetWorldTimerManager();
		if (!TimerManager.IsTimerActive(DetonationDelayTimer)
			|| TimerManager.GetTimerRemaining(DetonationDelayTimer) > Delay){

			TimerManager.SetTimer(DetonationDelayTimer,this, &AMissile::DelayedDetonation, Delay, false);			
		}
		return;
	}

	// detonate missile
	DetonateMissile(GetActorTransform());
}

void AMissile::DeactivateForDuration_Implementation(const float Duration)
{
}

FMissileStatus AMissile::GetCurrentMissileStatus_Implementation()
{
	FMissileStatus Status;
	Status.Target = CurrentTarget ? CurrentTarget->GetOwner() : nullptr;
	Status.RemainingBoostDistance = mRemainingBoostDistance;
	Status.bIsHoming = mHomingStatus != EHomingType::None;
	return Status;
}

// Sets default values
AMissile::AMissile(const FObjectInitializer& PCIP) : Super(PCIP)
{
	// Tick
	PrimaryActorTick.bCanEverTick = true;
	SetTickGroup(TG_PrePhysics);

	// Network
	SetReplicates(true);
	bNetUseOwnerRelevancy = true;
	NetCullDistanceSquared = FMath::Square(100000.0f); // Distance of 1km
	NetUpdateFrequency = 10.0f;
	MinNetUpdateFrequency = 1.0f;

	// the skeletal Mesh
	Mesh = PCIP.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("MissileMesh"));
	Mesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh->bReceivesDecals = false;
	Mesh->CastShadow = false;
	Mesh->bCastDynamicShadow = false;
	Mesh->SetCollisionProfileName(TEXT("Missile"));

	Mesh->OnComponentHit.AddDynamic(this, &AMissile::OnMeshHit);

	RootComponent = Mesh;

	// A sphere that acts as explosionradius/targetdetection
	ActorDetectionSphere = PCIP.CreateDefaultSubobject<USphereComponent>(this, TEXT("ActorDetection"));
	ActorDetectionSphere->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	ActorDetectionSphere->SetCollisionProfileName(TEXT("OverlapAll"));
	ActorDetectionSphere->SetCollisionObjectType(ECollisionChannel::ECC_Camera);
	ActorDetectionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ActorDetectionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	ActorDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AMissile::OnDetectionBeginOverlap);

	// Missileboostersoundeffect
	mBoosterSound = PCIP.CreateDefaultSubobject<UAudioComponent>(this, TEXT("EngineSound"));
	mBoosterSound->bAutoActivate = true;

	// Missiletrailparticlesystem
	mMissileTrail = PCIP.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("mMissileTrail"));
	mMissileTrail->bAutoActivate = false;

	BoosterSocket = "booster";
	ShockWaveVelocity = 30000.0f;

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
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// do stuff e.g.
	//AdvancedMissileMinRange = MaxVelocity;
}
#endif

// replication of variables
void AMissile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AMissile, mProperties, COND_InitialOnly);

	DOREPLIFETIME_CONDITION(AMissile, InitialVelocity, COND_InitialOnly);
	DOREPLIFETIME(AMissile, MissileLock);
	DOREPLIFETIME_CONDITION(AMissile, bBombingMode, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AMissile, BombingTargetLocation, COND_InitialOnly);

	DOREPLIFETIME(AMissile, CurrentTarget);
	DOREPLIFETIME_CONDITION(AMissile, SpiralHoming, COND_InitialOnly);
	DOREPLIFETIME(AMissile, CustomSpiralOffset);
	DOREPLIFETIME(AMissile, SpiralDirection);
	DOREPLIFETIME(AMissile, SpiralStrength);
	DOREPLIFETIME(AMissile, SpiralVelocity);
	DOREPLIFETIME_CONDITION(AMissile, SpiralDeactivationDistance, COND_InitialOnly);

	DOREPLIFETIME(AMissile, bDetonated);

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
	if (Role == ROLE_Authority)
	{
		bHadATarget = CurrentTarget ? true : false;
		CreateSpiralingBehaviour();
	}

	// clients and authority
	if (bBombingMode)
	{
		//AdvancedHoming = false;
		MissileLock = true;
		CurrentTargetLocation = BombingTargetLocation;
	}
	if (CurrentTarget) {
		TargetInfo.Actor = CurrentTarget->GetOwner();
	}

	// check: can accelerate
	bCanAccelerate = false;
	if (mProperties.AccelerationTime > 0.0f)
	{
		CurrentVelocity = InitialVelocity;
		Acceleration = 1.0f / mProperties.AccelerationTime;
		GetWorldTimerManager().SetTimer(AccelStartTimer, this, &AMissile::EnableAcceleration, mProperties.BoosterIgnitionDelay, false);
	}
	else
	{
		CurrentVelocity = mProperties.MaxVelocity;
		Acceleration = 0.0f;
	}
}

// Called every frame
void AMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CheckTargetTargetable();

	// count missile lifetime
	LifeTime += DeltaTime;

	DecreaseRemainingBoostDistance(DeltaTime);

	// client and server
	if (bBombingMode)
	{
		DirectionToTarget = CurrentTargetLocation - GetActorLocation();
		DistanceToTarget = DirectionToTarget.Size();
		DirectionToTarget.Normalize();
	}
	else
	{
		if (CurrentTarget)
		{
			CurrentTargetLocation = CurrentTarget->GetComponentLocation();
			DirectionToTarget = CurrentTargetLocation - GetActorLocation();
			DistanceToTarget = DirectionToTarget.Size();
			DirectionToTarget.Normalize();
		}
	}

	// perform homing to the target by rotating, both clients and server
	if (MissileLock) Homing(DeltaTime);

	// the distance the missile will be moved at the end of the current tick
	MovementVector = GetActorForwardVector() * DeltaTime * CurrentVelocity;

	if (bCanAccelerate)
	{
		if (!bReachedMaxVelocity)
		{
			// inrease Velocity
			CurrentVelocity += Acceleration * DeltaTime * mProperties.MaxVelocity;

			// inrease Turnrate
			Turnrate += Acceleration * DeltaTime * mProperties.HomingProperties.MaxTurnrate;

			// has reached max velocity?
			if (CurrentVelocity > mProperties.MaxVelocity)
			{
				CurrentVelocity = mProperties.MaxVelocity;
				Turnrate = mProperties.HomingProperties.MaxTurnrate;
				bReachedMaxVelocity = true;
				bCanAccelerate = false;
			}
		}
	}

	// server only
	if (HasAuthority())
	{
		//if (!CurrentTarget && bHadATarget)
		//{
		//	DetonateMissile();
		//}

		FCollisionObjectQueryParams TraceParams;
		TraceParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
		FHitResult HitResult;
		if (GetWorld()->LineTraceSingleByObjectType(HitResult, GetActorLocation(), GetActorLocation() + MovementVector, TraceParams))
		{
			SetActorLocation(HitResult.ImpactPoint);
			LOGA("Missile: LineTrace HitActor = \"%s\"", *HitResult.Actor->GetName())
				DetonateMissile(GetActorTransform());
		}
		else
		{
			// perform movement
			AddActorWorldOffset(MovementVector);
			// current missile transform for replication to client
		}
		MissileTransformOnAuthority = GetTransform();
	}
	else {
		// perform movement with correction
		if (LocationCorrectionTimeLeft > 0.0f)
		{
			AddActorWorldOffset(MovementVector + ClientLocationError * DeltaTime);
			LocationCorrectionTimeLeft -= DeltaTime;
		}
		else
		{
			AddActorWorldOffset(MovementVector);
		}
	}

	SmokeDrift(DeltaTime);

	// store current location for next Tick
	LastActorLocation = GetActorLocation();
	bNotFirstTick = true;
}

void AMissile::SmokeDrift(const float DeltaTime)
{
	if (mMissileTrail && GetWorld())
	{
		if (SmokeInterpAlpha > 1.0f)
		{
			SmokeInterpAlpha = 0.0f;
			SmokeRollAngleTarget += FMath::FRandRange(0.0f, 360.0f);
			CurrentInitialVelocityDirection = TargetInitialVelocityDirection;
			TargetInitialVelocityDirection = FRotator(0.0f, 0.0f, SmokeRollAngleTarget).RotateVector(FVector(-1.0f, 0.0f, 1.0f));
			SmokeRollInterpSpeed = FMath::FRandRange(3.0f, 5.0f);
		}

		SmokeInterpAlpha += DeltaTime * SmokeRollInterpSpeed;

		const float alpha = UCalcFunctionLibrary::FEaseInOutSin(0.0f, 1.0f, SmokeInterpAlpha);

		const FVector Dir = FMath::Lerp(CurrentInitialVelocityDirection, TargetInitialVelocityDirection, alpha);

		mMissileTrail->SetVectorParameter(FName("SmokeDrift"), Dir);
	}
}

void AMissile::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return; // is not server
	if (OtherActor && 
		 (OtherActor->Instigator != nullptr
		&& OtherActor->Instigator == Instigator))
		return; // other actor is owned by the same Player

	LOG("Missile: OnDetectionBeginOverlap")
		if (OtherActor)
		{
			LOGA("Missile: overlapping with %s", *OtherActor->GetName());
			if (CurrentTarget)
			{
				AActor* TargetActor = CurrentTarget->GetOwner();

				// overlapped the target?
				if (OtherActor == TargetActor)
				{
					// was target a missile?
					if (OtherActor->GetClass()->ImplementsInterface(UMissile_Interface::StaticClass()))
					{
						IMissile_Interface::Execute_Explode(OtherActor, this, 0.0f);
					}

					// explode
					DetonateMissile(GetActorTransform());
				}
			}
		}
}

void AMissile::MissileDestruction(AActor* actor)
{
	//
}

void AMissile::DecreaseRemainingBoostDistance(const float DeltaTime, const float BoostIntensity)
{
	mRemainingBoostDistance -= mProperties.MaxVelocity * DeltaTime * BoostIntensity;
	if (mRemainingBoostDistance < 0.0f)
	{
		MaxBoostRangeReached();
	}
}

void AMissile::MaxBoostRangeReached()
{
	if (HasAuthority())
	{
		DetonateMissile(GetActorTransform());
	}
}

void AMissile::CheckTargetTargetable()
{
	if (CurrentTarget)
	{
		AActor* TargetActor = CurrentTarget->GetOwner();
		if (TargetActor && TargetActor->GetClass()->ImplementsInterface(UTarget_Interface::StaticClass()))
		{
			if (!ITarget_Interface::Execute_GetIsTargetable(TargetActor, this))
			{
				CurrentTarget = nullptr;
				// reduce Boostdistance to cause an earlier explosion
				mRemainingBoostDistance *= 0.2f;
			}
		}
	}
}

void AMissile::CreateSpiralingBehaviour()
{
	// Rotation offset
	if (CustomSpiralOffset != 0.0f)
	{
		CustomSpiralOffset = FMath::FRandRange(0.0f, 360.f);
	}
	// Rotation direction cw/ccw
	if (SpiralDirection != 0)
	{
		SpiralDirection = FMath::Sign(SpiralDirection);
	}
	else
	{
		SpiralDirection = (FMath::RandBool()) ? -1.0f : 1.0f;
	}
	// Rotationrate
	if (RandomizeSpiralVelocity)
	{
		SpiralVelocity *= FMath::FRandRange(0.5f, 1.5f);
	}
}

void AMissile::DetonateMissile(const FTransform& Transform)
{
	if (!bDetonated && HasAuthority())
	{
		bDetonated = true;
		// make sure the missile looses its target
		CurrentTarget = nullptr;

		if (ActorDetectionSphere)
		{
			// cause all missiles overlapping this missiles detectionsphere to also explode (chain reaction) 
			TArray<AActor*> OverlappingActors;
			ActorDetectionSphere->GetOverlappingActors(OverlappingActors);

			LOGA("Missile: Overlapping Num = %d", OverlappingActors.Num())
				int32 num = 0;

			for (AActor* OverlapActor : OverlappingActors)
			{
				if (OverlapActor) {
					LOGA2("Missile: In Explosion Radius No: %d: \"%s\"", num, *OverlapActor->GetName())
						++num;
					if (OverlapActor->GetClass()->ImplementsInterface(UMissile_Interface::StaticClass()))
					{
						const float Distance = (OverlapActor->GetActorLocation() - GetActorLocation()).Size();
						const float Delay = FMath::GetMappedRangeValueUnclamped(FVector2D(0.0f, ShockWaveVelocity), FVector2D(0.0f, 1.0f), Distance);
						IMissile_Interface::Execute_Explode(OverlapActor, this, Delay);
					}
				}
			}

			// disable Overlap Events
			ActorDetectionSphere->OnComponentBeginOverlap.RemoveAll(this);

		}
		AllDetonateMissile(Transform);
		ExplodeMissile(Transform);
	}
}

void AMissile::DelayedDetonation()
{
	DetonateMissile(GetActorTransform());
}

void AMissile::OnDetonation_Implementation(const FTransform& Transform)
{
}

void AMissile::AllDetonateMissile_Implementation(const FTransform& ExplosionTransform)
{
	if (Role < ROLE_Authority)
	{
		ExplodeMissile(ExplosionTransform);
	}
}

void AMissile::HitTarget_Implementation(class AActor* TargetedActor)
{
	if (Role == ROLE_Authority && !bDetonated)
	{
		SetLifeSpan(mMissileTrailLifeSpan);
		if (bDamageTarget && CurrentTarget)
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Green, "Auth: Target HIT");
			CurrentTarget->GetOwner()->ReceiveAnyDamage(FMath::RandRange(mProperties.BaseDamage.MinDamage, mProperties.BaseDamage.MinDamage), nullptr, GetInstigatorController(), this);
			bDetonated = true;
			DetonateMissile(GetActorTransform());
		}
	}
}

void AMissile::ExplodeMissile(const FTransform& ExplosionTransform)
{
	SetActorTickEnabled(false);
	OnDetonation(ExplosionTransform);

	if (mBoosterSound) {
		mBoosterSound->Deactivate();
	}
	if (Mesh) {
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetHiddenInGame(true, false);
	}
	if (ActorDetectionSphere) {
		ActorDetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (mMissileTrail) {
		mMissileTrail->DeactivateSystem();
	}
}

void AMissile::OverlappingATarget(class AActor* OtherActor/*, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult*/)
{
	if (Role == ROLE_Authority)
	{
		if (bBombingMode && OtherActor)
		{
			//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.0f/*seconds*/, FColor::Green, "Overlapping in bombing mode");
			if (OtherActor == GetOwner()) return;
			CurrentTarget = OtherActor->GetRootComponent();
			bDamageTarget = true;
			HitTarget(CurrentTarget ? ((CurrentTarget->GetOwner()) ? CurrentTarget->GetOwner() : nullptr) : nullptr);
		}
	}
}

void AMissile::OnMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	LOG("Missile: OnMeshHit")
		DetonateMissile(GetActorTransform());
	Mesh->OnComponentHit.RemoveAll(this);
}

void AMissile::EnableAcceleration()
{
	bCanAccelerate = true;
}

void AMissile::OnRep_MissileTransformOnAuthority()
{
	if (Role < ROLE_Authority)
	{
		SetActorRotation(MissileTransformOnAuthority.GetRotation()); // correct Actor rotation
		ClientLocationError = (MissileTransformOnAuthority.GetLocation() - GetActorLocation()) * NetUpdateFrequency; // get location error scaled by replication frequency
		LocationCorrectionTimeLeft = NetUpdateInterval;
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f/*seconds*/, FColor::Red, "LocError: " + FString::SanitizeFloat(ClientLocationError.Size()) + "cm");
	}
}

// perform homing to the target by rotating
void AMissile::Homing(float DeltaTime)
{
	if (!CurrentTarget && !bBombingMode) return; // no homing when there is no valid target

	// is target prediction active?
	if (mProperties.HomingProperties.Homing == EHomingType::Advanced && !LastTargetLocation.IsZero())
	{
		//TargetVelocity = CurrentTarget->ComponentVelocity;
		TargetVelocity = (CurrentTargetLocation - LastTargetLocation) / DeltaTime; // A vector with v(x,y,z) = [cm/s]
		// store current targetlocation for next recalculation of target velocity
		FVelocity TargetVel;
		TargetVel.PreviousLocation = LastTargetLocation;
		TargetVel.CurrentLocation = CurrentTargetLocation;
		// calculate the location where missile and target will hit each other
		//PredictedTargetLocation = LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVelocity, Velocity);
		UCalcFunctionLibrary::LinearTargetPrediction(CurrentTargetLocation, GetActorLocation(), TargetVel, DeltaTime, FVector::ZeroVector, CurrentVelocity, PredictedTargetLocation);

		LastTargetLocation = CurrentTargetLocation;

		// a factor (0.0f - 1.0f) so that the missile is only following the target when far away and is predicting the targetlocation when close
		AdvancedHomingStrength = FMath::GetMappedRangeValueClamped(
			FVector2D(mProperties.HomingProperties.ActivationDistance, mProperties.HomingProperties.DistanceCompletelyActive),
			FVector2D(0.0f, 1.0f),
			DistanceToTarget);

		// calculate the new forward vector of the missile by taking the distance to the target into consideration 
		PredictedTargetLocation = FMath::Lerp(CurrentTargetLocation, PredictedTargetLocation, AdvancedHomingStrength);

		const FVector DirToPredictedTargetLocation = (PredictedTargetLocation - GetActorLocation()).GetSafeNormal();

		if (SpiralHoming && DistanceToTarget > SpiralDeactivationDistance)
		{
			float Amplitude = DistanceToTarget * SpiralStrength;

			const FVector NormalVec_To_DirToPredictedTargetLocationVector = FRotationMatrix(DirToPredictedTargetLocation.Rotation()).GetScaledAxis(EAxis::Y);
			// TODO: combine SpiralVelocity and SpiralDirection
			const float SpiralRotation = SpiralVelocity * LifeTime * SpiralDirection + CustomSpiralOffset;

			const FVector SpiralOffsetVector = (Amplitude * NormalVec_To_DirToPredictedTargetLocationVector).RotateAngleAxis(SpiralRotation, DirToPredictedTargetLocation);

			HomingLocation = PredictedTargetLocation + SpiralOffsetVector;
			DirectionToTarget = (HomingLocation - GetActorLocation()).GetSafeNormal();
		}
	}
	else
	{
		LastTargetLocation = CurrentTargetLocation;
		if (SpiralHoming && DistanceToTarget > SpiralDeactivationDistance)
		{
			const float BaseRotation = SpiralVelocity * LifeTime;
			const float NewRotation = int(BaseRotation + CustomSpiralOffset) % 360;
			const FRotator DirToTarget = DirectionToTarget.Rotation();
			const FVector VectorToRotate = FRotationMatrix(DirToTarget).GetScaledAxis(EAxis::Y);
			const FVector RotatedVector = VectorToRotate.RotateAngleAxis(NewRotation * SpiralDirection, DirectionToTarget);

			const float Amplitude = DistanceToTarget * SpiralStrength;
			HomingLocation = CurrentTargetLocation + Amplitude * RotatedVector;

			DirectionToTarget = (HomingLocation - GetActorLocation()).GetSafeNormal();

			/*
						HomingLocation = CurrentTargetLocation
			+ (Amplitude
			* FRotationMatrix(DirectionToTarget.Rotation()).GetScaledAxis(EAxis::Y)).RotateAngleAxis(
			(int(SpiralVelocity * LifeTime) % int(360))
			* SpiralDirection
			+ CustomSpiralOffset
			, DirectionToTarget);

			*/
		}
	}

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

//----------------------------------------------------- TESTING ------------------------------------------------

// testing
// replication of the timercalled funtion
void AMissile::RunsOnAllClients()
{
	if (Role == ROLE_Authority)
	{
		ServerRunsOnAllClients();
	}
}

// multicasted function
void AMissile::ServerRunsOnAllClients_Implementation()
{
	if (Role < ROLE_Authority)
	{
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
void AMissile::Dealing()
{
	if (Role == ROLE_Authority)
	{
		ServerDealing();
	}
}

// testing
void AMissile::ServerDealing_Implementation()
{
	//
}

float AMissile::DistanceLineLine(const FVector& a1,
	const FVector& a2,
	const FVector& b1,
	const FVector& b2)
{
	return ((a1 - a2) * (b1 ^ b2)).Size() / (b1 ^ b2).Size();
}

//returns the Distance or -1 / WIP
bool AMissile::ClosestPointsOnTwoLines(const FVector& LineStartA,
	const FVector& LineEndA,
	const FVector& LineStartB,
	const FVector& LineEndB,
	FVector& PointA,
	FVector& PointB)
{
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

	return (t * t < 1.0f && s * s < 1.0f);
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

void AMissile::RunsOnOwningClientOnly()
{
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
