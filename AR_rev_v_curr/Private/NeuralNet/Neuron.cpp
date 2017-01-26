// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#define LOG_MSG 0
#include "CustomMacros.h"
#include "Neuron.h"

Connection::Connection(const float weight)
{
	this->weight = weight;
	deltaWeight = 0.0f;
}

Neuron::Neuron(const int32 numOutputs, const int32 myIndex)
{
	this->myIndex = myIndex;
	for (int i = 0; i < numOutputs; ++i)
	{
		const Connection con = Connection(Neuron::RandomWeight());
		outputWeights.Add(con);
	}
}

Neuron::Neuron(const int32 myIndex, const TArray<float>& genome)
{
	this->myIndex = myIndex;
	for (float weight : genome)
	{
		const Connection con = Connection(weight);
		outputWeights.Add(con);
	}
}

int Neuron::getNumConnections() const
{
	return outputWeights.Num();
}

bool Neuron::setGenome(const TArray<float>& genome)
{
	if (genome.Num() != outputWeights.Num())
	{
		LOGA2("Neuron: setGenome: Array-sizes are different! Failed to set Genome: %d != %d", genome.Num(), outputWeights.Num())
		return false;
	}
	for (int i = 0; i < genome.Num(); ++i)
	{
		LOGA2("Neuron: setGenome: prev Gen = %lf; new Gen = %lf", outputWeights[i].weight,genome[i])
		outputWeights[i].weight = genome[i];		
	}
	return true;
}

TArray<float> Neuron::getGenome() const
{
	TArray<float> genome;
	for (Connection con : outputWeights)
	{
		genome.Add(con.weight);
	}
	return genome;
}

float Neuron::getOutputValue() const
{
	return OutputValue;
}

void Neuron::setOutputValue(const float newOutputValue)
{
	OutputValue = newOutputValue;
}

float Neuron::RandomWeight()
{
	return FMath::RandRange(-1.0f, 1.0f);
}

void Neuron::feedForward(const Layer& previousLayer)
{
	LOG("Neuron: feedForward:")
	float sum = 0.0f;
	int32 runCount = 0;
	// get the sum of the outputs of the previous Layer (inclusive Bias-Neuron)
	for (int32 i = 0; i < previousLayer.Num(); ++i)
	{
		if (previousLayer[i].outputWeights.IsValidIndex(myIndex)) {
			const float myWeight = previousLayer[i].outputWeights[myIndex].weight;
			const float connectionOutput = previousLayer[i].getOutputValue();
			LOGA2("Neuron: feedForward: myWeight = %lf; connectionOutput = %lf", myWeight, connectionOutput)

			sum += connectionOutput * myWeight;
			++runCount;
		} else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Neuron: OutputWeights empty!");
		}
	}
	LOGA("Neuron: feedForward: collected weights of %d connections", runCount)
	setOutputValue(transferFunction(sum));
}

float Neuron::transferFunction(const float x)
{
	// tanh - output-range [-1.0...1.0]
	const float value = tanh(x);
	LOGA2("Neuron: transferFunction: new OutputValue = %lf from input = %lf", value, x)
	return value;
}

float Neuron::transferFunctionDerivative(const float x)
{
	// tanh derivative (approximation)
	return 1.0f - FMath::Square(x);
}

void Neuron::calcOutputGradient(const float targetVal)
{
	const float delta = targetVal - getOutputValue();
	Gradient = delta * transferFunctionDerivative(getOutputValue());
}

void Neuron::calcHiddenGradient(const Layer& nextLayer)
{
	const float dow = sumDOW(nextLayer);
	Gradient = dow * transferFunctionDerivative(getOutputValue());
}

float Neuron::sumDOW(const Layer& nextLayer) const
{
	float sum = 0;
	// sum the contributions of the errors at the Neurons that are fed
	for (int n = 0; n < nextLayer.Num() - 1; ++n)
	{
		sum += outputWeights[n].weight * nextLayer[n].Gradient;
	}
	return sum;
}

void Neuron::updateInputWeights(const Layer& prevLayer) const
{
	// the weights to be updated are in the connection container in the neurons in the preceding layer
	for (int n = 0; n < prevLayer.Num(); ++n)
	{
		Neuron neuron = prevLayer[n];
		double oldDeltaWeight = neuron.outputWeights[myIndex].deltaWeight;
		double newDeltaWeight =
			// individual input, magnified by the gradient and train rate
			eta
			* neuron.getOutputValue()
			* Gradient
			// also add momentum = a fraction of the previous delta weight
			+ alpha
			* oldDeltaWeight;

		neuron.outputWeights[myIndex].deltaWeight = newDeltaWeight;
		neuron.outputWeights[myIndex].weight = neuron.outputWeights[myIndex].weight + newDeltaWeight;
	}
}
