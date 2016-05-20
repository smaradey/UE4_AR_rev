// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "CustomRadialForceComponent.h"


// Sets default values for this component's properties
UCustomRadialForceComponent::UCustomRadialForceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Radius = 200.0f;
	Falloff = RIF_Constant;
	ImpulseStrength = 1000.0f;
	ForceStrength = 10.0f;
	bAutoActivate = true;

	// by default we affect all 'dynamic' objects that can currently be affected by forces
	AddCollisionChannelToAffect(ECC_Pawn);
	AddCollisionChannelToAffect(ECC_PhysicsBody);
	AddCollisionChannelToAffect(ECC_Vehicle);
	AddCollisionChannelToAffect(ECC_Destructible);

	UpdateCollisionObjectQueryParams();
}

// Called every frame
void UCustomRadialForceComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
	if (bIsActive)
	{
		const FVector Origin = GetComponentLocation();

		// Find objects within the sphere
		static FName AddForceOverlapName = FName(TEXT("AddForceOverlap"));
		TArray<FOverlapResult> Overlaps;

		FCollisionQueryParams Params(AddForceOverlapName, false);
		Params.bTraceAsyncScene = true; // want to hurt stuff in async scene

										// Ignore owner actor if desired
		if (bIgnoreOwningActor)
		{
			Params.AddIgnoredActor(GetOwner());
		}

		GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, CollisionObjectQueryParams, FCollisionShape::MakeSphere(Radius), Params);

		// Iterate over each and apply force
		for (int32 OverlapIdx = 0; OverlapIdx<Overlaps.Num(); OverlapIdx++)
		{
			UPrimitiveComponent* PokeComp = Overlaps[OverlapIdx].Component.Get();
			if (PokeComp)
			{
				PokeComp->AddRadialForce(Origin, Radius, ForceStrength, Falloff);

				// see if this is a target for a movement component
				AActor* PokeOwner = PokeComp->GetOwner();
				if (PokeOwner)
				{
					TInlineComponentArray<UMovementComponent*> MovementComponents;
					PokeOwner->GetComponents<UMovementComponent>(MovementComponents);
					for (const auto& MovementComponent : MovementComponents)
					{
						if (MovementComponent->UpdatedComponent == PokeComp)
						{
							MovementComponent->AddRadialForce(Origin, Radius, ForceStrength, Falloff);
							break;
						}
					}
				}
			}
		}
	}
}

void UCustomRadialForceComponent::PostLoad()
{
	Super::PostLoad();

	UpdateCollisionObjectQueryParams();
}


void UCustomRadialForceComponent::AddCollisionChannelToAffect(enum ECollisionChannel CollisionChannel)
{
	EObjectTypeQuery ObjectType = UEngineTypes::ConvertToObjectType(CollisionChannel);
	if (ObjectType != ObjectTypeQuery_MAX)
	{
		AddObjectTypeToAffect(ObjectType);
	}
}

void UCustomRadialForceComponent::AddObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType)
{
	ObjectTypesToAffect.AddUnique(ObjectType);
	UpdateCollisionObjectQueryParams();
}

void UCustomRadialForceComponent::RemoveObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType)
{
	ObjectTypesToAffect.Remove(ObjectType);
	UpdateCollisionObjectQueryParams();
}

#if WITH_EDITOR

void UCustomRadialForceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// If we have edited the object types to effect, update our bitfield.
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == TEXT("ObjectTypesToAffect"))
	{
		UpdateCollisionObjectQueryParams();
	}
}

#endif

void UCustomRadialForceComponent::UpdateCollisionObjectQueryParams()
{
	CollisionObjectQueryParams = FCollisionObjectQueryParams(ObjectTypesToAffect);
}


void UCustomRadialForceComponent::FireImpulse()
{
	const FVector Origin = GetComponentLocation();

	// Find objects within the sphere
	static FName FireImpulseOverlapName = FName(TEXT("FireImpulseOverlap"));
	TArray<FOverlapResult> Overlaps;

	FCollisionQueryParams Params(FireImpulseOverlapName, false);
	Params.bTraceAsyncScene = true; // want to hurt stuff in async scene

									// Ignore owner actor if desired
	if (bIgnoreOwningActor)
	{
		Params.AddIgnoredActor(GetOwner());
	}

	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, CollisionObjectQueryParams, FCollisionShape::MakeSphere(Radius), Params);

	// Iterate over each and apply an impulse
	for (int32 OverlapIdx = 0; OverlapIdx<Overlaps.Num(); OverlapIdx++)
	{
		UPrimitiveComponent* PokeComp = Overlaps[OverlapIdx].Component.Get();
		if (PokeComp)
		{
			// If DestructibleDamage is non-zero, see if this is a destructible, and do damage if so.
			if (DestructibleDamage > SMALL_NUMBER)
			{
				UDestructibleComponent* DestructibleComp = Cast<UDestructibleComponent>(PokeComp);
				if (DestructibleComp != NULL)
				{
					DestructibleComp->ApplyRadiusDamage(DestructibleDamage, Origin, Radius, ImpulseStrength, (Falloff == RIF_Constant));
				}
			}

			// Do impulse after
			PokeComp->AddRadialImpulse(Origin, Radius, ImpulseStrength, Falloff, bImpulseVelChange);

			// see if this is a target for a movement component
			TInlineComponentArray<UMovementComponent*> MovementComponents;
			PokeComp->GetOwner()->GetComponents<UMovementComponent>(MovementComponents);
			for (const auto& MovementComponent : MovementComponents)
			{
				if (MovementComponent->UpdatedComponent == PokeComp)
				{
					MovementComponent->AddRadialImpulse(Origin, Radius, ImpulseStrength, Falloff, bImpulseVelChange);
					break;
				}
			}
		}
	}
}