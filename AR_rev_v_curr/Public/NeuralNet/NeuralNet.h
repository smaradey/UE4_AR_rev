// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AR_rev_v_curr.h"
#include "Neuron.h"
#include "BiasNeuron.h"
#include "Components/ActorComponent.h"
#define LOG_MSG 0
#include "CustomMacros.h"
#include "NeuralNet.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class AR_REV_V_CURR_API UNeuralNet : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	//UNeuralNet();

	UNeuralNet(const FObjectInitializer &ObjectInitializer);

	/* Creates the Neural Network: creates all the Layers and Neurons and initializes them with random weights */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	void InitNet(const TArray<int>& topology);

	/* the Fitness-Value of this specific Neural-Network */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	float getFitness() const;

	/* set the Fitness-Value of this specific Neural-Network to a new Value */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	void setFitness(const float newFitness);

	/* returns the current Error of the Neural-Network */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	float getError() const;

	/* returns the recent average Error of the Neural-Network */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	float getRecentAverageError() const;

	/* pushes the input through the Neural-Network, to get the results call getResult(), when training the net with Back-Propagation call first backProp() and after that getResults(), returns true on Success */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	bool feedForward(const TArray<float>& inputValues);

	/* returns an Array with the Float-Values of the Neural-Nets OutputLayer */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	TArray<float> getResults();

	/* returns an Array with all the weights of the whole Neural-Network, can only be used with other Neural-Networks that have the SAME Topology (Num. Layers, Num. Neurons/Layer) */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	TArray<float> getGenome();

	/* attempts to feed a given Array with weights in to the Neural-Network, replacing all existing weights, returns true on Success */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	bool setGenome(const TArray<float>& genome);

	/* takes an Array of floats and turns it into a String of float-Values, separated by "," */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	static FString convertFloatArrayToCSVString(const TArray<float>& genome);

	/* takes a String of float-Values, separated by ",", and turns it into an Array of floats */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	static TArray<float> convertCSVStringToFloatArray(const FString& Str);

	/* returns the Lifetime of this Neural Network */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Neural Net")
	float getLifeTime() const;

	/* sets the "Birth" of this Neural Network to the current Game-Time */
	UFUNCTION(BlueprintCallable, Category = "Neural Net")
	void resetLifeTime();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Net Debug")
		bool DEBUG = false;

	/* 1st Element: Number of Inputneurons in the first Layer; last Element: Number of Outputneurons in the last Layer; Elements between 1st and last: hidden Layers with specified Number of Neurons */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Neural Net Properties")
	TArray<int32> Topology;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Neural Net Properties")
	bool bUseCustomGenome = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "Neural Net Properties")
	FString CustomCSVGenome;

private:
	UPROPERTY()
		float recentAverageSmoothingFactor = 10.0f;
	float fitness = 0.0f;
	float error;
	float recentAverageError;
	TArray<Layer> NeuronLayers;
	float BirthTime;
	bool bInitialized;
};
