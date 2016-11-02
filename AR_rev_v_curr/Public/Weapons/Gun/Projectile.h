// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Damage.h"
#include "Projectile_Enums.h"
#include "Projectile_Structs.h"
#include "Projectile.generated.h"


UCLASS()
class AR_REV_V_CURR_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AProjectile();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	// struct that holds all important properties that defines the projectiles behaviour
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Projectile|Settings")
		FProjectileProperties ProjectileProperties;

	void SetProjectileProperties(const FProjectileProperties& Properties)
	{
		ProjectileProperties = Properties;
	}



};
