// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AR_rev_v_curr.h"
#include "Neuron.h"

/**
 * 
 */
class AR_REV_V_CURR_API BiasNeuron : public Neuron
{
public:
	BiasNeuron(const int32 numOutputs, const int32 myIndex);
	BiasNeuron(const int32 myIndex, const TArray<float> &genome);

	virtual float getOutputValue() const override;
};
