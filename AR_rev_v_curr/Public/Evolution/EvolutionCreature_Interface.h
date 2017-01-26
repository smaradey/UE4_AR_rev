// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Evolution_Structs.h"
#include "EvolutionCreature_Interface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEvolutionCreature_Interface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class AR_REV_V_CURR_API IEvolutionCreature_Interface
{
	GENERATED_IINTERFACE_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EvolutionCreature_Interface|NeuralNet")
	FEvolutionCreature GetNeuralNetData();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "EvolutionCreature_Interface|NeuralNet")
		void UpdateCreature(const FEvolutionCreature& Creature, const bool bRandomGenome);
	
};
