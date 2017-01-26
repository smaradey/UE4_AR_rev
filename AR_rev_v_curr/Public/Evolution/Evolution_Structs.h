// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AR_rev_v_curr.h"
#include "Evolution_Structs.generated.h"

USTRUCT(BlueprintType)
struct FEvolutionCreature {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Creature")
		int32 CreatureID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Creature")
	TSubclassOf<AActor> CreatureClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Creature|NeuralNet")
		float Fitness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Creature|NeuralNet")
		TArray<int32> Topology;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Creature|NeuralNet")
		TArray<float> Weights;
};


USTRUCT(BlueprintType)
struct FEvolutionCreaturePool {
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool|Creature")
		TSubclassOf<AActor> CreatureClass;	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool|Creature|NeuralNet")
		TArray<int32> Topology;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool")
		int32 MaxLiving = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool")
		int32 MaxPoolSize = 50;

	// Key: Fitness, Value: Creature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool")
		TArray<FEvolutionCreature> FitnessPool;
};
