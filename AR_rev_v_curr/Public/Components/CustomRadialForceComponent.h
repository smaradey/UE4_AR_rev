// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PhysicsEngine/RadialForceComponent.h"
#include "PhysicsEngine/RadialForceActor.h"
#include "Components/SceneComponent.h"
#include "CustomRadialForceComponent.generated.h"


UCLASS(hidecategories = (Object, Mobility, LOD, Physics), ClassGroup = Physics, showcategories = Trigger, meta = (BlueprintSpawnableComponent))
class AR_REV_V_CURR_API UCustomRadialForceComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

		/** The radius to apply the force or impulse in */
		UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category = RadialForceComponent)
		float Radius;

	/** How the force or impulse should fall off as object are further away from the center */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadialForceComponent)
		TEnumAsByte<enum ERadialImpulseFalloff> Falloff;

	/** How strong the impulse should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impulse)
		float ImpulseStrength;

	/** If true, the impulse will ignore mass of objects and will always result in a fixed velocity change */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impulse)
		uint32 bImpulseVelChange : 1;


	/** If true, do not apply force/impulse to any physics objects that are part of the Actor that owns this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Impulse)
		uint32 bIgnoreOwningActor : 1;

	/** How strong the force should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Force)
		float ForceStrength;

	/** If true, the force applied will act as acceleration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Force)
		uint32 bAccelChange : 1;

	/** If > 0.f, will cause damage to destructible meshes as well  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Destructible)
		float DestructibleDamage;

	/** Fire a single impulse */
	UFUNCTION(BlueprintCallable, Category = "Physics|Components|RadialForce")
		virtual void FireImpulse();

	/** Add an object type for this radial force to affect */
	UFUNCTION(BlueprintCallable, Category = "Physics|Components|RadialForce")
		virtual void AddObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType);

	/** Remove an object type that is affected by this radial force */
	UFUNCTION(BlueprintCallable, Category = "Physics|Components|RadialForce")
		virtual void RemoveObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType);

	/** Add a collision channel for this radial force to affect */
	void AddCollisionChannelToAffect(enum ECollisionChannel CollisionChannel);

protected:
	/** The object types that are affected by this radial force */
	UPROPERTY(EditAnywhere, Category = RadialForceComponent)
		TArray<TEnumAsByte<enum EObjectTypeQuery> > ObjectTypesToAffect;

	/** Cached object query params derived from ObjectTypesToAffect */
	FCollisionObjectQueryParams CollisionObjectQueryParams;

protected:
	//~ Begin UActorComponent Interface.
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	//~ End UActorComponent Interface.

	//~ Begin UObject Interface.
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface.

	/** Update CollisionObjectQueryParams from ObjectTypesToAffect */
	void UpdateCollisionObjectQueryParams();

private:
	UPrimitiveComponent *OwnerRoot;
	const float GravitationConstant = 6.67408E-11f;
};
