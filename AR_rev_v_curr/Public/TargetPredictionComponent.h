// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "TargetPredictionComponent.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AR_REV_V_CURR_API UTargetPredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTargetPredictionComponent();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Target Prediction|Target")
		void SetTarget(class UPrimitiveComponent* NewTarget)
	{
		Target = NewTarget;
		InitVelocityToZero();

	}

	UFUNCTION(BlueprintCallable, Category = "Target Prediction|Target")
		FVector GetTargetVelocity()
	{
		return Target ? TargetVelocity : FVector::ZeroVector;
	}

	// Forces the component to set the Velocity to zero.
	// Sets the previous Target Location to the Current Target Location
	// Useful in case the target can teleport.
	UFUNCTION(BlueprintCallable, Category = "Target Prediction|Target")
		void InitVelocityToZero()
	{
		if (Target)
		{
			OldTargetLocation = CurrentTargetLocation = Target->GetComponentLocation();
			TargetVelocity = FVector::ZeroVector;
			//SetComponentTickEnabled(true);
		}
		else
		{
			OldTargetLocation = CurrentTargetLocation = TargetVelocity = FVector::ZeroVector;
			//SetComponentTickEnabled(false);
		}
	}

	UFUNCTION(BlueprintCallable, Category = "Target Prediction|Predicted Location")
		FVector PredictTargetLocation(const FVector& StartLocation, const FVector& AdditionalVelocity, const float ProjectileVelocity, const float DeltaTime)
	{
		// linear targetprediction
		const FVector AB = (CurrentTargetLocation - StartLocation).GetSafeNormal();
		const FVector Velocity = TargetVelocity - AdditionalVelocity;
		const FVector vi = Velocity - (FVector::DotProduct(AB, Velocity) * AB);
		return StartLocation + vi + AB * FMath::Sqrt(FMath::Square(ProjectileVelocity) - FMath::Pow((vi.Size()), 2.f));
	}

	UFUNCTION(BlueprintCallable, Category = "Target Prediction")
		void Update(class UPrimitiveComponent* CurrentTarget, const float DeltaTime)
	{
		if (CurrentTarget != Target)
		{
			SetTarget(CurrentTarget);
			return;
		}
		if (Target)
		{
			OldTargetLocation = CurrentTargetLocation;
			CurrentTargetLocation = Target->GetComponentLocation();

			if (DeltaTime != 0.0f)
			{
				TargetVelocity = (CurrentTargetLocation - OldTargetLocation) / DeltaTime;
			}
			else
			{
				TargetVelocity = FVector::ZeroVector;
			}
		}
		else
		{
			InitVelocityToZero();
		}
	}




private:

	class UPrimitiveComponent* Target;
	FVector OldTargetLocation;
	FVector CurrentTargetLocation;
	FVector TargetVelocity;
};
