// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "TargetPredictionComponent.h"


// Sets default values for this component's properties
UTargetPredictionComponent::UTargetPredictionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = false;
	bReplicates = true;
}


// Called when the game starts
void UTargetPredictionComponent::BeginPlay()
{
	Super::BeginPlay();

	InitVelocityToZero();	
}


// Called every frame
void UTargetPredictionComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	
}



