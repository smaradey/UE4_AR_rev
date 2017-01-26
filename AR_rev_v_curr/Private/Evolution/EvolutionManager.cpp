// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "EvolutionManager.h"
#include "Neuron.h"


// Sets default values
AEvolutionManager::AEvolutionManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEvolutionManager::BeginPlay()
{
	Super::BeginPlay();

	LoadCreatePools();
	FTimerHandle CheckTimer;
	GetWorldTimerManager().SetTimer(CheckTimer, this, &AEvolutionManager::CheckPools, CheckInterval, true);
	FTimerHandle SaveTimer;
	GetWorldTimerManager().SetTimer(SaveTimer, this, &AEvolutionManager::SaveAllPools, SaveInterval, true);
}

// Called every frame
void AEvolutionManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AEvolutionManager::GenerateSaveGameName(const FEvolutionCreaturePool& CreaturePool, FString& SaveGameSlotName)
{
	SaveGameSlotName = UEvolutionProgress::GetBaseSaveSlotName();
	if (CreaturePool.CreatureClass)
	{
		SaveGameSlotName.Append("_CLASS_" + CreaturePool.CreatureClass->GetName());
	}
	if (CreaturePool.Topology.Num() > 0)
	{
		SaveGameSlotName.Append("_TOP");
		for (int32 numNeurons : CreaturePool.Topology)
		{
			SaveGameSlotName.AppendChar('_');
			SaveGameSlotName.AppendInt(numNeurons);
		}
		LOGA("EvolutionManager: Generated SaveGameName: %s", *SaveGameSlotName)
			return true;
	}
	LOG("EvolutionManager: GenerateSaveGameName: Topology is not set up correctly in CreaturePool!")
		return false;
}

UEvolutionProgress* AEvolutionManager::LoadProgress(const FString& SlotName) const
{
	UEvolutionProgress* LoadGameInstance = CreateProgressSaveGame();
	LoadGameInstance = Cast<UEvolutionProgress>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	return LoadGameInstance;
}

void AEvolutionManager::SaveAllPools()
{
	for (FEvolutionCreatureSettings& Settings : Creatures)
	{
		Settings.SaveGame->Pool = Settings.Creature;
		bool rc = SaveProgress(Settings.SaveGame);
		UE_LOG(LogTemp, Log, TEXT("EvolutionManger: SaveAllPools: Saving succeeded = %d; %s"), rc, *Settings.SaveGame->SaveSlotName);
	}
}

UEvolutionProgress* AEvolutionManager::CreateProgressSaveGame() const
{
	return Cast<UEvolutionProgress>(UGameplayStatics::CreateSaveGameObject(UEvolutionProgress::StaticClass()));
}

bool AEvolutionManager::SaveProgress(UEvolutionProgress* Progress)
{
	if (Progress)
	{
		return UGameplayStatics::SaveGameToSlot(Progress, Progress->SaveSlotName, 0);
	}
	LOG("EvolutionManager: SaveProgress: Progress is NULL")
		return false;
}

bool AEvolutionManager::DeleteProgress(const FString& SlotName)
{
	return false;
}

void AEvolutionManager::SpawnCreature()
{
}

void AEvolutionManager::GenerateNewGenome(const TArray<float>& CreatureAGenomes, const TArray<float>& CreatureBGenomes, TArray<float>& GeneratedGenomes, const float ProbabilityOneMutation)
{
	LOG("EvolutionManger: GenerateNewGenome:")

		if (CreatureAGenomes.Num() == 0 || CreatureBGenomes.Num() == 0)
		{
			LOG("EvolutionManager: GenerateNewGenome: A or B is empty!")
				GeneratedGenomes.Empty();
			return;
		}

	if (CreatureAGenomes.Num() != CreatureBGenomes.Num())
	{
		LOG("EvolutionManager: GenerateNewGenome: Size of A and B is not equal")
			GeneratedGenomes.Empty();
		return;
	}
	GeneratedGenomes.Empty(CreatureAGenomes.Num());
	GeneratedGenomes.SetNum(CreatureAGenomes.Num(), true);
	const float MutationProbPerWeight = ProbabilityOneMutation / CreatureAGenomes.Num();
	LOGA2("EvolutionManager: GenerateNewGenome: ProbSingleMutation = %lf; ProbPerWeight = %lf", ProbabilityOneMutation, MutationProbPerWeight)
		int32 mutCnt = 0;
	for (int32 i = 0; i < CreatureAGenomes.Num(); ++i)
	{

		if (FMath::FRand() < MutationProbPerWeight)
		{

			++mutCnt;
			GeneratedGenomes[i] = Neuron::RandomWeight();
		}
		else
		{
			GeneratedGenomes[i] = FMath::RandBool() ? CreatureAGenomes[i] : CreatureBGenomes[i];
		}
	}
#if DEBUG_MSG == 1
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1,0.5f, FColor::Red, "Number of Mutations = " + FString::FromInt(mutCnt));
#endif
}

void AEvolutionManager::LoadCreatePools()
{
	for (FEvolutionCreatureSettings& Settings : Creatures)
	{
		UEvolutionProgress* Progress = CreateProgressSaveGame();
		if (GenerateSaveGameName(Settings.Creature, Progress->SaveSlotName))
		{
			const bool SaveGameExists = UGameplayStatics::DoesSaveGameExist(Progress->SaveSlotName, 0);
			if (SaveGameExists)
			{
				LOGA("EvolutionManager: BeginPlay: Progress Exists: %s", *Progress->SaveSlotName)
					UEvolutionProgress* Loaded = LoadProgress(Progress->SaveSlotName);
				if (Loaded)
				{
					Settings.SaveGame = Loaded;
					LOGA("EvolutionManager: BeginPlay: Loaded: %s", *Settings.SaveGame->SaveSlotName)
						continue;
				}
			}
			LOGA("EvolutionManager: BeginPlay: Progress does not yet exist: %s", *Progress->SaveSlotName)
				Settings.SaveGame = Progress;
			if (SaveProgress(Settings.SaveGame))
			{
				LOGA("EvolutionManager: BeginPlay: Saved: %s", *Settings.SaveGame->SaveSlotName)
			}
			else
			{
				LOGA("EvolutionManager: BeginPlay: Saving failed: %s", *Progress->SaveSlotName)
			}
		}
	}
	for (FEvolutionCreatureSettings& Settings : Creatures)
	{
		Settings.Creature.FitnessPool = Settings.SaveGame->Pool.FitnessPool;
	}
}

void AEvolutionManager::CheckPools()
{
	LOG("EvolutionManger: CheckPools")

		for (FEvolutionCreatureSettings& Settings : Creatures)
		{
			//Settings.SaveGame->Pool = Settings.Creature;
			//bool rc = SaveProgress(Settings.SaveGame);
			//LOGA("EvolutionManger: CheckPools: Saving Succeeded = %d", rc)

				TSubclassOf<AActor>& CreatureClass = Settings.Creature.CreatureClass;
			const int32& MaxNumLiving = Settings.Creature.MaxLiving;

			int32 NumLivingCreatures = 0;
			UWorld* World = GetWorld();
			if (CreatureClass && World)
			{
				for (TActorIterator<AActor> It(World, CreatureClass); It; ++It)
				{
					AActor* Actor = *It;
					if (!Actor->IsPendingKill())
					{
						++NumLivingCreatures;
					}
				}
				UE_LOG(LogTemp, Log, TEXT("AEvolutionManager::CheckPools Number of living creatures = %d, Server-DeltaTime = %lf seconds"), NumLivingCreatures, World->DeltaTimeSeconds)
			}
			

			const int32 NumSpawnable = FMath::Min(SpawnAmount, MaxNumLiving - NumLivingCreatures);
			if (NumSpawnable > 0)
			{
				TArray<FEvolutionCreature>& PooledCreatures = Settings.Creature.FitnessPool;

				const float avg = GetAverageFitness(PooledCreatures);
				LOGA("EvolutionManger: CheckPools: average Fitness = %lf", avg)
#if DEBUG_MSG == 1
					if (GEngine) GEngine->AddOnScreenDebugMessage(-1, CheckInterval, FColor::Red, "EvolutionManger: CheckPools: average Fitness = " + FString::SanitizeFloat(avg));
#endif

				LOGA2("EvolutionManger: CheckPools: Spawnable of class %s are %d", *CreatureClass->GetName(), NumSpawnable)
					for (int32 i = 0; i < NumSpawnable; ++i)
					{
						if (Settings.SpawnPoints.Num() > 0)
						{
							const ATargetPoint* SpawnPoint = Settings.SpawnPoints[FMath::RandRange(0, Settings.SpawnPoints.Num() - 1)];

							// create new Creature
							FEvolutionCreature BornCreature;
							BornCreature.CreatureClass = CreatureClass;
							BornCreature.CreatureID = 0; // TODO: incrementing
							BornCreature.Fitness = 0.0f;
							BornCreature.Topology = Settings.Creature.Topology;

							// Generate new Genome/Weights
							bool RandomGenome = false;
							if (PooledCreatures.Num() == Settings.Creature.MaxPoolSize || (PooledCreatures.IsValidIndex(0) && PooledCreatures.Num() < Settings.Creature.MaxPoolSize && !bSpawnRandomCreaturesToFillPool))
							{
								const int32 indexA = FMath::RandRange(0, PooledCreatures.Num() - 1);
								const int32 indexB = FMath::RandRange(0, PooledCreatures.Num() - 1);

								TArray<float>& gensA = PooledCreatures[indexA].Weights;
								TArray<float>& gensB = PooledCreatures[indexB].Weights;
								GenerateNewGenome(gensA, gensB, BornCreature.Weights, MutationProbability);
							}
							else
							{
								LOGA("EvolutionManger: CheckPools: Creaturepool contains no creatures. Num = %d", PooledCreatures.Num())
									// random Genome
									RandomGenome = true;
							}

							LOG("EvolutionManger: CheckPools: Spawning...")
								Spawning(SpawnPoint->GetTransform(), BornCreature, RandomGenome);
						}
						else
						{
							LOG("EvolutionManger: CheckPools: Spawnpoints are not set!")
						}
					}
			}
		}
}

void AEvolutionManager::AddCreatureToPool(const FEvolutionCreature& Creature)
{
	LOG("EvolutionManger: AddCreatureToPool:")
		for (FEvolutionCreatureSettings& Settings : Creatures)
		{
			if (Creature.CreatureClass == Settings.Creature.CreatureClass)
			{
				if (Creature.Topology.Num() == Settings.Creature.Topology.Num())
				{
					bool SameTopology = true;
					for (int32 i = 0; i < Creature.Topology.Num(); ++i)
					{
						if (Creature.Topology[i] != Settings.Creature.Topology[i])
						{
							SameTopology = false;
							break;
						}
					}
					if (!SameTopology) continue;
				}
				else
				{
					continue;
				}

				Settings.Creature.FitnessPool.Add(Creature);
				// TODO: insert using binary search
				//InsertCreature(Settings.Creature.FitnessPool, Creature);

				LOGA("EvolutionManger: AddCreatureToPool: MaxPoolSize = %d", Settings.Creature.MaxPoolSize)
					while (Settings.Creature.FitnessPool.Num() > Settings.Creature.MaxPoolSize)
					{
						int32 worst = -1;
						for (int32 i = 0; i < Settings.Creature.FitnessPool.Num(); ++i)
						{
							if (worst < 0)
							{
								worst = i;
								continue;
							}
							else if (Settings.Creature.FitnessPool[i].Fitness < Settings.Creature.FitnessPool[worst].Fitness)
							{
								worst = i;
							}
						}

						if (Settings.Creature.FitnessPool.IsValidIndex(worst))
						{
							LOGA("EvolutionManger: AddCreatureToPool: Removing creature with fitness = %lf", Settings.Creature.FitnessPool[worst].Fitness)
								Settings.Creature.FitnessPool.RemoveAt(worst);
						}
					}

				break;
			}
		}
}

void AEvolutionManager::InsertCreature(TArray<FEvolutionCreature>& FitnessPool, const FEvolutionCreature& Creature)
{
	TArray<FEvolutionCreature> Result;
	if (FitnessPool.Num() < 1)
	{
		FitnessPool.Add(Creature);
		LOG("EvolutionManger: InsertCreature: Creatures inserted at index = 0")
			return;
	}
	int32 index = 0;
	while (FitnessPool.IsValidIndex(index) && FitnessPool[index].Fitness >= Creature.Fitness)
	{
		Result.Add(FitnessPool[index]);
		++index;
	}
	Result.Add(Creature);
	LOGA2("EvolutionManger: InsertCreature: Creatures inserted at index = %d with Fitness = %f", index, Creature.Fitness)
		for (index; FitnessPool.IsValidIndex(index); ++index)
		{
			Result.Add(FitnessPool[index]);
		}
	LOGA("EvolutionManger: InsertCreature: Creatures in Pool = %d", Result.Num())
		FitnessPool = Result;
	//InsertCreature(FitnessPool, Creature, 0, FitnessPool.Num() - 1);
}

void AEvolutionManager::InsertCreature(TArray<FEvolutionCreature>& FitnessPool, const FEvolutionCreature& Creature, const int32 LeftIndex, const int32 RightIndex)
{
	if (LeftIndex > RightIndex)
	{
		FitnessPool.Insert(Creature, RightIndex);
		LOGA("EvolutionManger: InsertCreature: Creatures inserted at index = %d", RightIndex)
			return;
	}

	const int32 CenterIndex = (LeftIndex + RightIndex) / 2;

	if (FitnessPool[LeftIndex].Fitness > Creature.Fitness && Creature.Fitness > FitnessPool[CenterIndex].Fitness)
	{
		InsertCreature(FitnessPool, Creature, LeftIndex + 1, CenterIndex);
	}
	else
	{
		InsertCreature(FitnessPool, Creature, CenterIndex + 1, RightIndex);
	}
}

float AEvolutionManager::GetAverageFitness(const TArray<FEvolutionCreature>& PooledCreatures)
{
	if (PooledCreatures.Num() < 1)
	{
		LOG("EvolutionManager: GetAverageFitness: Pool is empty!")
		return 0;
	}
	float avg = 0;
	float Best = 0;
	float Worst = 0;
	const float factor = 1.0f / PooledCreatures.Num();
	for (const FEvolutionCreature& Creature : PooledCreatures)
	{
		if (Creature.Fitness < Worst || Worst == 0.0f) Worst = Creature.Fitness;
		if (Creature.Fitness > Best) Best = Creature.Fitness;
		avg += Creature.Fitness * factor;
	}
	UE_LOG(LogTemp, Log, TEXT("AEvolutionManager::GetAverageFitness = %lf, Poolsize = %d, Best = %lf, Worst = %lf"), avg, PooledCreatures.Num(), Best, Worst);
	return avg;
}

void AEvolutionManager::Spawning_Implementation(const FTransform& SpawnTransform, const FEvolutionCreature& Creature, const bool bRandomGenome)
{
}
