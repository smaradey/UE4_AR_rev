// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "NeuralNet.h"

// Sets default values for this component's properties
UNeuralNet::UNeuralNet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetComponentTickEnabled(false);
}

void UNeuralNet::InitNet(const TArray<int32>& topology)
{
	resetLifeTime();

	const int32 numLayers = topology.Num();
	LOGA("NeuralNet: InitNet: topology length = %d", topology.Num())
		// check for existing layers
		if (numLayers < 1)
		{
			LOG("NeuralNet: InitNet: Number of Layers in topology is invalid!")
				return;
		}

	// check for invalid Values: x < 1
	for (int32 numNeurons : topology)
	{
		if (numNeurons < 1)
		{
			LOGA("NeuralNet: InitNet: Number of Neurons %d is invalid!", numNeurons)
				return;
		}
	}

	for (int layerIndex = 0; layerIndex < numLayers; ++layerIndex)
	{
		// create the new Layer
		Layer newLayer;

		// get the number of non-Bias-Neuron of the next Layer, if the current Layer is the Output-Layer the number of Outputs is 0
		const int32 numOutputs = layerIndex == numLayers - 1 ? 0 : topology[layerIndex + 1];

		// fill the layer with Neurons, add one Bias per Layer
		for (int i = 0; i <= topology[layerIndex]; ++i)
		{
			// the last Element is a Bias-Neuron
			if (i == topology[layerIndex])
			{
				newLayer.Add(BiasNeuron(numOutputs, i));
				LOGA("NeuralNet: InitNet: Added one Bias-Neuron: Index = %d", i)
					if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "Neural Net: Init: Added one Bias-Neuron: Index = " + FString::FromInt(i));
			}
			else
			{
				newLayer.Add(Neuron(numOutputs, i));
				LOGA("NeuralNet: InitNet: Added one normal Neuron: Index = %d", i)
					if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, "Neural Net: Init: Added one Neuron: Index = " + FString::FromInt(i));
			}
		}
		// add the new Layer to the LayerList
		NeuronLayers.Add(newLayer);
		LOGA2("NeuralNet: InitNet: Added one Layer: Index = %d; Number of Neurons in Layer = %d", layerIndex, newLayer.Num())
			if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Blue, "Neural Net: Init: Added one Layer: Index = " + FString::FromInt(layerIndex) + "; Number of Neurons in Layer = " + FString::FromInt(newLayer.Num()));
	}
	bInitialized = true;

	if (bUseCustomGenome)
	{
		bool rc = setGenome(convertCSVStringToFloatArray(CustomCSVGenome));
		if (!rc && GEngine) {
			GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, "Neural Net: Set Custom Genome FAILED");
		LOG("NeuralNet: InitNet: Set Custom Genome FAILED")
		}
	}
}

float UNeuralNet::getFitness() const
{
		return fitness;
}

void UNeuralNet::setFitness(const float newFitness)
{
	this->fitness = newFitness;
}

float UNeuralNet::getError() const
{
	return error;
}

float UNeuralNet::getRecentAverageError() const
{
	return recentAverageError;
}

bool UNeuralNet::feedForward(const TArray<float>& inputValues)
{
	//LOG("NeuralNet: feedForward")

	// check whether NeuronLayers contains data
	if (NeuronLayers.Num() < 1) {
		LOG("NeuralNet: feedForward: Number of Layers in NeuronLayers is invalid!")
			return false;
	}

	Layer& InputNeuronLayer = NeuronLayers[0];

	// check whether the input data matches the number of input Neurons (without Bias-Neuron)
	if (inputValues.Num() != InputNeuronLayer.Num() - 1)
	{
		LOG("Neural Net: feedForward: InputLength (without Bias) does not match length of InputLayer!")
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Neural Net: feedForward: InputLength (without Bias) does not match length of InputLayer ");
		return false;
	}

	// put the input values into the input neurons
	for (int i = 0; i < inputValues.Num(); ++i)
	{
		if(FMath::Abs(inputValues[i]) > 1.0f)
		{
			LOG("Neural Net: feedForward: InputValue is larger/smaller 1.0! ERROR!!! Make sure your inputs are normalized!")
				return false;
		}
		InputNeuronLayer[i].setOutputValue(inputValues[i]);
	}

	// forward propagate
	for (int layerIndex = 1; layerIndex < NeuronLayers.Num(); ++layerIndex)
	{
		// get a reference to the previous and the current Layer
		const Layer& prevLayer = NeuronLayers[layerIndex - 1];
		Layer& currLayer = NeuronLayers[layerIndex];
		// go through all Neurons (except Bias-Neuron) in the current Layer and feed forward
		for (int n = 0; n < currLayer.Num() - 1; ++n)
		{
			currLayer[n].feedForward(prevLayer);
		}
	}
	return true;
}

TArray<float> UNeuralNet::getResults()
{
	TArray<float> resultVals;
	if (NeuronLayers.Num() <= 0)
	{
		LOG("NeuralNet: getResults: no Neuron-Layers exist");
		return resultVals;
	}
	const int32 lastIndex = NeuronLayers.Num() - 1;
	for (int n = 0; n < NeuronLayers[lastIndex].Num() - 1; ++n)
	{
		resultVals.Add(NeuronLayers[lastIndex][n].getOutputValue());
	}
	return resultVals;
}

TArray<float> UNeuralNet::getGenome()
{
	TArray<float> genome;
	if (!bInitialized) return genome;

	if (NeuronLayers.Num() < 1)
	{
		if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, "Neural Net: getGenome: Genome is Empty");
		return genome;
	}

	for (Layer& layer : NeuronLayers)
	{
		if (layer.Num() < 1)
		{
			if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, "Neural Net: getGenome: Layer is empty!");
			return genome;
		}

		for (Neuron& node : layer)
		{
			genome.Append(node.getGenome());
		}
	}
	if (DEBUG && GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, "Neural Net: getGenome: Found " + FString::FromInt(genome.Num()) + " Gens.");
	return genome;
}

bool UNeuralNet::setGenome(const TArray<float>& genome)
{
	LOG("NeuralNet: setGenome:");
	int32 index = 0;

	if (genome.Num() < 1)
	{
		LOG("NeuralNet: setGenome: new Genome contains nothing");
		return false;
	}

	if (NeuronLayers.Num() < 1)
	{
		LOG("Neural Net: setGenome: no Neuron-Layers exist");
		return false;
	}

	for (Layer& layer : NeuronLayers)
	{
		if (layer.Num() < 1)
		{
			LOG("Neural Net: setGenome: Layer is empty!");
			return false;
		}

		for (Neuron& node : layer)
		{
			TArray<float> subGenome;
			for (int i = 0; i < node.getNumConnections(); ++i)
			{
				subGenome.Add(genome[index]);
				++index;
			}
			if (!node.setGenome(subGenome)) {
				LOG("Neural Net: setGenomeInSubGenome: FAILED");
				return false;
			}
		}
	}
	return true;
}

FString UNeuralNet::convertFloatArrayToCSVString(const TArray<float>& genome)
{
	if (genome.Num() == 0)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, "Neural Net: convertGenomeToString: Genome is empty!");
		return FString("NONE");
	}
	FString str = FString::SanitizeFloat(genome[0]);
	for (int i = 1; i < genome.Num(); ++i)
	{
		str.Append("," + FString::SanitizeFloat(genome[i]));
	}
	return str;
}

TArray<float> UNeuralNet::convertCSVStringToFloatArray(const FString& Str)
{
	TArray<float> result;
	TArray<FString> GenStrings;
	const FString delim = ",";
	Str.ParseIntoArray(GenStrings, *delim, true);

	for (FString& GenString : GenStrings)
	{
		result.Add(FCString::Atof(*GenString));
	}
	return result;
}

float UNeuralNet::getLifeTime() const
{
	return GetWorld()->TimeSeconds - BirthTime;
}

void UNeuralNet::resetLifeTime()
{
	BirthTime = GetWorld()->TimeSeconds;
}

// Called when the game starts
void UNeuralNet::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UNeuralNet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
