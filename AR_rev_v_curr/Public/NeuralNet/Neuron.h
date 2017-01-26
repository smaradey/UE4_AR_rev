// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AR_rev_v_curr.h"
#include <cmath>

typedef TArray<class Neuron> Layer;


struct Connection
{
	float weight;
	float deltaWeight;

	Connection(const float weight);
};

/**
 *
 */
class AR_REV_V_CURR_API Neuron
{
public:
	/* Constructor that creates the Number of Connections and the Index of this Neuron in it's Neuron-Layer */
	Neuron(const int32 numOutputs, const int32 myIndex);

	/* Constructor that initializes the Connection to the next Neuron-Layer with custom weights, also takes the Index of this Neuron in it's Neuron-Layer*/
	Neuron(const int32 myIndex, const TArray<float>& genome);

	/* returns the Number of connections this Neuron has to the next Neuron-Layer */
	int getNumConnections() const;

	/* sets the weights of this neurons Connections, returns true on Success */
	bool setGenome(const TArray<float>& genome);

	/* returns an Array<float> with all the weights of this Neuron */
	TArray<float> getGenome() const;

	/* returns this Neurons Output */
	virtual float getOutputValue() const;

	/* setter for this Neurons OutputValue */
	void setOutputValue(const float newOutputValue);

	/* returns a random value in range of [-1.0f..1.0f] */
	static float RandomWeight();

	/* calculates this Neurons new Output from the previous Layers Outputs and Connection-Weights */
	void feedForward(const Layer& previousLayer);

	/* the Transfer-Function of the Neuron (Hyperbolic Tangent) */
	static float transferFunction(const float x);

	/* the Derivative of the Transfer-Function (approximated) */
	static float transferFunctionDerivative(const float x);

	/* calculates this Neurons Output Gradient */
	void calcOutputGradient(const float targetVal);

	void calcHiddenGradient(const Layer& nextLayer);

	float sumDOW(const Layer& nextLayer) const;

	void updateInputWeights(const Layer& prevLayer) const;

private:
	// overall net learning rate: 0 slow learner, 0.2 medium learner, 1.0 reckless learner [0.0..1.0]
		const float eta = 0.1f;
	// multiplier of last weight change: 0 no momentum, 0.5 moderate momentum [0.0..n]
		const float alpha = 0.5f;

	float OutputValue = 1.0f;
	float Gradient = 0.0f;
	int32 myIndex = 0;
	TArray<Connection> outputWeights;
};
