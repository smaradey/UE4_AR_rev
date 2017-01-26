// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/GameMode.h"
#include "FlightGameMode.generated.h"

/**
 * 
 */
UCLASS()
class AR_REV_V_CURR_API AFlightGameMode : public AGameMode
{
	GENERATED_BODY()
	
protected:
	AFlightGameMode(const FObjectInitializer& ObjectInitializer);

	virtual FString InitNewPlayer(class APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	
};
