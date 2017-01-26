// Fill out your copyright notice in the Description page of Project Settings.

#include "AR_rev_v_curr.h"
#include "FlightGameMode.h"
#include "FlightPlayerController.h"
#include "MainPawn.h"


AFlightGameMode::AFlightGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// set the default classes
	DefaultPawnClass = AMainPawn::StaticClass();
	PlayerControllerClass = AFlightPlayerController::StaticClass();
	//PlayerStateClass = AFlightPlayerState::StaticClass();
	//GameStateClass = AFlightGameState::StaticClass();
	SpectatorClass = ASpectatorPawn::StaticClass();


}

FString AFlightGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	/*ASPlayerState* NewPlayerState = Cast<ASPlayerState>(NewPlayerController->PlayerState);
	if (NewPlayerState)
	{
		NewPlayerState->SetTeamNumber(PlayerTeamNum);
	}*/

	return Result;
}

AActor* AFlightGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	/* Get all playerstarts */
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	
	/* Pick a random spawnpoint from the filtered spawn points */
	APlayerStart* Start = Cast<APlayerStart>(PlayerStarts[FMath::RandHelper(PlayerStarts.Num())]);;
	
	/* If we failed to find any (so BestStart is nullptr) fall back to the base code */
	return Start ? Start : Super::ChoosePlayerStart_Implementation(Player);
}
