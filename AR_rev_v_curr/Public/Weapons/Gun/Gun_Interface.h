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

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|Input")
		void RequestFire();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|Input")
		void StopFireRequest();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|Input")
		void RequestReload();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|Input")
		void StopReloadRequest();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|Input")
		void AddAmmunition(const int32 Amount);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void WeaponStatusChanged(const FWeaponStatus& WeaponStatus);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface")
		void FireProjectile(const FVector& SpreadDirection, const FProjectileProperties& Projectile, const bool Tracer);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|StatusUpdates")
		void StartedReloading(const float& ReloadTime);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|StatusUpdates")
		void FinishedReloading();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|StatusUpdates")
		void AbortedReloading();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|StatusUpdates")
		void Overheated();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|StatusUpdates")
		void NoLongerOverheated();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "GunFireInterface|FPS_Recoil")
		APawn* GetOwningPawn();




};
