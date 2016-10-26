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

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void RequestFire();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void StopFireRequest();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void RequestReload();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void StopReloadRequest();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void AddAmmunition(const int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void WeaponStatusChanged(const FWeaponStatus& WeaponStatus);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void FireProjectile(const FVector& SpreadDirection, const FProjectileProperties& Projectile, const bool Tracer);
};
