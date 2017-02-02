// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Explosive_Interface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UExplosive_Interface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class AR_REV_V_CURR_API IExplosive_Interface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Explosive_Interface")
		void Detonated(const FTransform& Transform);
	
};
