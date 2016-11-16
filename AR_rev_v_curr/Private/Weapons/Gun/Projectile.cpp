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
	BounceThreshold = -0.8f;

	// private
	Bounces = 0;
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	Velocity = GetActorForwardVector() * ProjectileProperties.MuzzleVelocity + AdditionalVelocity;

	bUnlimitedBouncing = MaxBounces < 0;

	const float newLifeSpan = ProjectileProperties.MaxRange / FMath::Max(1.0f, ProjectileProperties.MuzzleVelocity);
	SetLifeSpan(newLifeSpan);
	LOGA("Projectile: BeginPlay: Set Lifespan to %f", newLifeSpan);

	// update the rotation to point in the direction of the velocity
	SetActorRotation(Velocity.Rotation(), ETeleportType::TeleportPhysics);

	// TODO: recalc Tracer Length

	OnBounce();
	// Initial Random Movement for more realistic Projectile Movement
	const FVector TempVelocity = Velocity;
	// TODO: make Seeded random
	Velocity *= FMath::Rand;
	Movement();
	Velocity = TempVelocity;
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Movement();
}

void AProjectile::OnBounce_Implementation()
{
}

void AProjectile::OnImpact_Implementation()
{
}

void AProjectile::Movement()
{
	TraceStartLocation = GetActorLocation();
	PendingTravel = 1.0f;
	do
	{
		bBounceAgain = false;
		TraceAfterBounce();
	} while (bBounceAgain);
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
#if DEBUG == 1
		const float DEBUGLineLifeTime = 5.0f;
#endif
		if (World->LineTraceSingleByObjectType(HitResult, TraceStartLocation, TraceEndLocation, TraceParams))
		{
#if DEBUG == 1
			// Red up to the blocking hit, green thereafter
			::DrawDebugLine(World, TraceStartLocation, HitResult.ImpactPoint, FColor::Red, false, DEBUGLineLifeTime);
			::DrawDebugLine(World, HitResult.ImpactPoint, TraceEndLocation, FColor::Green, false, DEBUGLineLifeTime);
			::DrawDebugPoint(World, HitResult.ImpactPoint, 16.f, FColor::Red, false, DEBUGLineLifeTime);
#endif
			HandleTraceResult(HitResult);
		}
		else
		{
#if DEBUG == 1
			// no hit means all red
			::DrawDebugLine(World, TraceStartLocation, TraceEndLocation, FColor::Red, false, DEBUGLineLifeTime);
#endif
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
	return FVector::DotProduct(Hit.Normal, Velocity.GetSafeNormal()) > BounceThreshold;
}

void AProjectile::Impact(const FHitResult& Hit)
{
	OnImpact();
	LOG("Projectile: Impact")
		Destroy();
}

void AProjectile::Bounce(const FHitResult& Hit)
{
	OnBounce();
	PendingTravel *= (1.0f - Hit.Time);
	TraceStartLocation = Hit.ImpactPoint; // + Hit.Normal;
	Velocity = Velocity.MirrorByVector(Hit.Normal) * Bounciness;
	++Bounces;
	if (Velocity.SizeSquared() < MinVelocitySquarred)
	{
		Destroy();
	}
	else
	{
		// TODO: Recalc Tracer Length
		bBounceAgain = true;
	}
}

void AProjectile::HandleTraceResult(const FHitResult& Hit)
{
	if (BouncingAllowed())
	{
		if (CanBounce(Hit))
		{
			Bounce(Hit);
		}
		else
		{
			Impact(Hit);
		}
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
