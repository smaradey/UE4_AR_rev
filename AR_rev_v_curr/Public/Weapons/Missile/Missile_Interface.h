// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Damage.h"
#include "Missile_Structs.h"
#include "Missile_Interface.generated.h"


UINTERFACE(Blueprintable, MinimalAPI)
class UMissile_Interface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class AR_REV_V_CURR_API IMissile_Interface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// @object class reference that calls the function
	// @Delay time in seconds
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Missile_Interface")
		void Explode(UObject* object, const float Delay);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Missile_Interface")
		void DeactivateForDuration(const float Duration);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Missile_Interface")
		FMissileStatus GetCurrentMissileStatus();
};
