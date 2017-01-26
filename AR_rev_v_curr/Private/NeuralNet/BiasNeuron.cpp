// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "AR_rev_v_curr.h"
#include "BiasNeuron.h"

BiasNeuron::BiasNeuron(const int32 numOutputs, const int32 myIndex) : Neuron(numOutputs, myIndex){
}

BiasNeuron::BiasNeuron(const int32 myIndex, const TArray<float> &genome) : Neuron(myIndex, genome){
}

float BiasNeuron::getOutputValue() const
{
	return 1.0f;
}

