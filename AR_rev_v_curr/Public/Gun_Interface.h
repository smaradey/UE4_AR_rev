//// Fill out your copyright notice in the Description page of Project Settings.
//
//#pragma once
//
///**
// * 
// */
//class AR_REV_V_CURR_API Gun_Interface
//{
//public:
//	Gun_Interface();
//	~Gun_Interface();
//};
//--------------------

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Projectile.h"
#include "GunFireComponent.h"
#include "Gun_Interface.generated.h"

/**
*
*/

UINTERFACE(Blueprintable, MinimalAPI)
class UGun_Interface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IGun_Interface
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireComponent")
		void ProjectileSpawned(const FProjectileProperties& Properties, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireComponent")
		void WeaponStatusChanged(const FWeaponStatus& WeaponStatus);


};
