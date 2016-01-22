// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Cpp_HowTos.generated.h"

UCLASS()
class AR_REV_V_CURR_API ACpp_HowTos : public AActor
{
public:
	GENERATED_BODY()

		// Making a property show up in the editor:
		UPROPERTY(EditAnywhere)
		int32 Number;

	UPROPERTY(EditAnywhere, Category = "Damage")
		int32 MinDamage;

	UPROPERTY(EditAnywhere, Category = "Damage")
		int32 MaxDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		int32 TotalDamage;

	// designer can modify
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
		float DamageTimeInSeconds;

	// calculated value using the designer’s settings
	// VisibleAnywhere flag marks that property as viewable,
	// but not editable
	// Transient flag means that it won’t be saved or loaded from disk;
	// it’s meant to be a derived, non-persistent value
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Damage")
		float DamagePerSecond;



	// Sets default values for this actor's properties
	ACpp_HowTos();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	
	
};
