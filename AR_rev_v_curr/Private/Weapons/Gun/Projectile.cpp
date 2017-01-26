// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "Projectile.h"

#define DEBUG 0


// Sets default values
AProjectile::AProjectile(const FObjectInitializer& PCIP) : Super(PCIP)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// public
	MinVelocitySquarred = 2500.0f;
	Bounciness = 0.25f;
	MaxBounces = 2;
	BounceThreshold = 0.6f;
	bUsePhysicalMaterial = true;

	// private
	Bounces = 0;
	bPendingDestruction = false;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	DEBUGStart = GetTransform();

	Velocity = GetActorForwardVector() * ProjectileProperties.MuzzleVelocity + AdditionalVelocity;

	bUnlimitedBouncing = MaxBounces < 0;

	const float newLifeSpan = ProjectileProperties.MaxRange / FMath::Max(1.0f, ProjectileProperties.MuzzleVelocity);
	SetLifeSpan(newLifeSpan);
	LOGA("Projectile: BeginPlay: Set Lifespan to %f", newLifeSpan);

	// update the rotation to point in the direction of the velocity
	SetActorRotation(Velocity.Rotation(), ETeleportType::TeleportPhysics);

	// TODO: recalc Tracer Length

	//OnBounce();
	// Initial Random Movement for more realistic Projectile Movement
	const FVector TempVelocity = Velocity;
	// TODO: make Seeded random
	//Velocity *= FMath::FRand();
	//Movement();
	//Velocity = TempVelocity;
	PendingTravel = FMath::FRand();
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bPendingDestruction)
	{
		Destroy();
	}
	else
	{
		Movement();
	}
}

void AProjectile::OnBounce_Implementation(const FHitResult& Hit)
{
}

void AProjectile::OnImpact_Implementation(const FHitResult& HitResult)
{
}

void AProjectile::Movement()
{
	// DEBUG
	if (!bCanMove)
	{
		SetActorTransform(DEBUGStart, false, nullptr, ETeleportType::TeleportPhysics);
		Velocity = GetActorForwardVector() * 100000.0f;
	}
	Locations.Empty(4);
	TraceStartLocation = GetActorLocation();
	Locations.Add(TraceStartLocation);


	int cnt = 0;
	do
	{
		bBounceAgain = false;
		TraceAfterBounce();
		++cnt;
		if (cnt > 100)
		{
			LOG("Projectile: TOO MANY COLLISIONS")
				Destroy();
			break;
		}
	} while (bBounceAgain);

	PendingTravel = 1.0f;
}

void AProjectile::TraceAfterBounce()
{
	const UWorld* const World = GetWorld();
	if (World)
	{
		switch (ProjectileProperties.ProjectileType)
		{
		case EProjectileType::Kinetic:
		{
			FVector Gravity = World ? FVector(0, 0, World->GetGravityZ()) : FVector::ZeroVector;
			UCalcFunctionLibrary::GetWorldGravity(this, Gravity);
			Velocity = Velocity + Gravity * (PendingTravel * World->DeltaTimeSeconds);
		}
		break;
		case EProjectileType::Energy: break;
		default: break;
		}
		TraceEndLocation = TraceStartLocation + Velocity * (World->DeltaTimeSeconds * PendingTravel);

		FCollisionObjectQueryParams TraceParams;
		TraceParams.AllObjects;
		TraceParams.RemoveObjectTypesToQuery(ECC_Projectile);
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.bFindInitialOverlaps = false;

		if (World->LineTraceSingleByObjectType(HitResult, TraceStartLocation, TraceEndLocation, TraceParams, QueryParams))
		{
#if DEBUG == 1
			// Red up to the blocking hit, green thereafter
			::DrawDebugLine(World, TraceStartLocation, HitResult.ImpactPoint, FColor::Red, false, DEBUGLineLifeTime);
			::DrawDebugLine(World, HitResult.ImpactPoint, TraceEndLocation, FColor::Green, false, DEBUGLineLifeTime);
			::DrawDebugPoint(World, HitResult.ImpactPoint, 16.f, FColor::Red, false, DEBUGLineLifeTime);
#endif
			Locations.Add(HitResult.ImpactPoint);
			HandleTraceResult(HitResult);
		}
		else
		{
#if DEBUG == 1
			// no hit means all red
			::DrawDebugLine(World, TraceStartLocation, TraceEndLocation, FColor::Red, false, DEBUGLineLifeTime);
#endif
			Locations.Add(TraceEndLocation);
			UpdateTransform();
		}
	}
}

bool AProjectile::BouncingAllowed()
{
	return bUnlimitedBouncing ^ (Bounces < MaxBounces);
}

bool AProjectile::CanBounce(const FHitResult& Hit)
{
	lastImpactDotProduct = FMath::Abs(FVector::DotProduct(Hit.Normal, Velocity.GetSafeNormal()));

	UPhysicalMaterial* PhysMatRef = Hit.PhysMaterial.Get();
	if(bUsePhysicalMaterial && PhysMatRef)
	{		
		LOGA("Projectile: CanBounce: PhysMat = %d", (int32) PhysMatRef->SurfaceType.GetValue())
		LOGA2("Projectile: CanBounce: Restitution = %f; Dot = %f", PhysMatRef->Restitution, lastImpactDotProduct)
		LOGA2("Projectile: CanBounce: Dot %f < %f ?", lastImpactDotProduct, BounceThreshold * PhysMatRef->Restitution)

		return lastImpactDotProduct < BounceThreshold * PhysMatRef->Restitution;
	} else
	{
		LOG("Projectile: CanBounce: NoPhysMat")

		return lastImpactDotProduct < BounceThreshold;
	}
}

void AProjectile::Impact(const FHitResult& Hit)
{
	OnImpact(Hit);
	LOG("Projectile: Impact")
		bPendingDestruction = true;
}

void AProjectile::Bounce(const FHitResult& Hit)
{
	OnBounce(Hit);
	PendingTravel *= (1.0f - Hit.Time);
	TraceStartLocation = Hit.ImpactPoint + Hit.Normal;
	UPhysicalMaterial* PhysMatRef = Hit.PhysMaterial.Get();
	if (bUsePhysicalMaterial && PhysMatRef)
	{
		const float CombinedBounciness = Bounciness * PhysMatRef->Restitution;
		Velocity = Velocity.MirrorByVector(Hit.Normal);

		const float VelocityMag = Velocity.Size() * CombinedBounciness * /* testing */ (1.0f - lastImpactDotProduct);
		Velocity.Normalize();

		const FVector PlaneDistanceVector = Velocity.ProjectOnToNormal(Hit.Normal);

		if(PhysMatRef->Friction < 1.0f)
		{
			Velocity = Velocity - PlaneDistanceVector * PhysMatRef->Friction;
			Velocity.Normalize();
			Velocity *= VelocityMag;
		} else
		{
			Velocity = Velocity + PlaneDistanceVector * PhysMatRef->Friction;
			const float FrictionLength = Velocity.Size();
			Velocity.Normalize();
			Velocity *= VelocityMag / FrictionLength;
		}


	}else
	{
		Velocity = Velocity.MirrorByVector(Hit.Normal) * Bounciness;
	}
	
	++Bounces;
	if (Velocity.SizeSquared() < MinVelocitySquarred)
	{
		bPendingDestruction = true;
	}
	else
	{
		// TODO: Recalc Tracer Length
		bBounceAgain = true;
	}
}

void AProjectile::HandleTraceResult(const FHitResult& Hit)
{
	if (BouncingAllowed() && CanBounce(Hit))
	{
		Bounce(Hit);
	}
	else
	{
		Impact(Hit);
	}
}

void AProjectile::UpdateTransform()
{
	const FRotator newRotation = (TraceEndLocation - TraceStartLocation).Rotation();
	SetActorLocationAndRotation(TraceEndLocation, newRotation, false, nullptr, ETeleportType::TeleportPhysics);
}
