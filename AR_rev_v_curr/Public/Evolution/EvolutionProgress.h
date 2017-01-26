// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AR_rev_v_curr.h"
#include "GameFramework/SaveGame.h"
#include "Evolution_Structs.h"
#include "EvolutionProgress.generated.h"


/**
 *
 */
UCLASS()
class AR_REV_V_CURR_API UEvolutionProgress : public USaveGame
{
	GENERATED_BODY()

public:


	UEvolutionProgress(const FObjectInitializer& ObjectInitializer)
		: USaveGame(ObjectInitializer)
	{
		SaveSlotName = GetBaseSaveSlotName();
	}	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Basic)
		FString SaveSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Basic)
		FEvolutionCreaturePool Pool;

	UFUNCTION(BlueprintCallable, Category = "EvolutionProgress|SaveGame")
	static FString GetBaseSaveSlotName();
};
