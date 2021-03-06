// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Target_Interface.generated.h"

/**
 * 
 */

UINTERFACE(Blueprintable, MinimalAPI)
class UTarget_Interface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ITarget_Interface
{
	GENERATED_IINTERFACE_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Target_Interface")
		bool GetIsTargetable(AActor* enemy);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Target_Interface")
		void StartTargetingActor(AActor* enemy);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Target_Interface")
		void StopTargetingActor(AActor* enemy);


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Target_Interface")
		void GetTargetPoints(TArray<ATargetPoint*>& TargetPoints);



};