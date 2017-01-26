// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Evolution_Structs.h"
#include "EvolutionCreature_Interface.h"
#include "EvolutionProgress.h"
#define LOG_MSG 0
#define DEBUG_MSG 1
#include "CustomMacros.h"
#include "EvolutionManager.generated.h"


USTRUCT(BlueprintType)
struct FEvolutionCreatureSettings
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Creature")
		FEvolutionCreaturePool Creature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Creature")
		TArray<ATargetPoint*> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager|Pool")
		UEvolutionProgress* SaveGame;
};

UCLASS()

class AR_REV_V_CURR_API AEvolutionManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEvolutionManager();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
		TArray<FEvolutionCreatureSettings> Creatures;

	// generate Creatures with random weights to fill the Pool, if false: create creatures from already existing ones in the pool (at least one must exist or the first one generated will be random)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
	bool bSpawnRandomCreaturesToFillPool = true;

	// 1.0 => 100% that one single Mutation occurs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
		float MutationProbability = 1.0f;

	// how often the Manager checks whether he can spawn creatures
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
		float CheckInterval = 1.0f;

	// how many creatures can be spawned at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
	int32 SpawnAmount = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution|Manager")
		int32 SaveInterval = 60;

	/**
	 * Creates the Name for the savegameslot using the topology of the Neuralnet as suffix.
	 * The suffix contains the layers of the Neuralnet where every Layer is represented by a Number which is also the Number of Neurons (without Bias-Neuron) in that Layer
	 */
	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
		bool GenerateSaveGameName(const FEvolutionCreaturePool& CreaturePool, FString& SaveGameSlotName);

	/**
	 * returns NULL if loading failes
	 */
	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
	UEvolutionProgress* LoadProgress(const FString& SlotName) const;

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
	void SaveAllPools();

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
		FORCEINLINE UEvolutionProgress* CreateProgressSaveGame() const;

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
		static bool SaveProgress(UEvolutionProgress* Progress);

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager|SaveGame")
		static bool DeleteProgress(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		static void SpawnCreature();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Evolution|Manager")
		void Spawning(const FTransform& SpawnTransform, const FEvolutionCreature& Creature, const bool bRandomGenome);


	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		/**
			 * \param CreatureAGenomes
			 * \param CreatureBGenomes
			 * \param GeneratedGenomes
			 * \param ProbabilityOneMutation probability that in the GeneratedGenomes is one mutated/random value, if equal to Number of elems the result will contain only mutated/random values
			 */
		static void GenerateNewGenome(const TArray<float>& CreatureAGenomes, const TArray<float>& CreatureBGenomes, TArray<float>& GeneratedGenomes, const float ProbabilityOneMutation = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		void LoadCreatePools();

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		void CheckPools();

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		void AddCreatureToPool(const FEvolutionCreature& Creature);

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
		void InsertCreature(TArray<FEvolutionCreature>& FitnessPool, const FEvolutionCreature& Creature);

	void InsertCreature(TArray<FEvolutionCreature>& FitnessPool, const FEvolutionCreature& Creature, const int32 LeftIndex, const int32 RightIndex);

	UFUNCTION(BlueprintCallable, Category = "Evolution|Manager")
	static float GetAverageFitness(const TArray<FEvolutionCreature>& PooledCreatures);
};
