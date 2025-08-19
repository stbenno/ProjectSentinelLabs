// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/Lobby/SFW_LobbyGameMode.h"
#include "Core/Game/Lobby/SFW_LobbyGameState.h"
#include "Core/Game/SFW_PlayerState.h"
#include "Core/Game/SFW_PlayerController.h"
#include "Net/UnrealNetwork.h"

ASFW_LobbyGameMode::ASFW_LobbyGameMode()
{
	//GameStateClass = ASFW_LobbyGameState::StaticClass();
	//PlayerControllerClass = ASFW_PlayerController::StaticClass();
	//PlayerStateClass = ASFW_PlayerState::StaticClass();
	//bUseSeamlessTravel = true;
}

void ASFW_LobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Ensure phase starts as Waiting (authority).
	if (ASFW_LobbyGameState* LGS = GetGameState<ASFW_LobbyGameState>())
	{
		LGS->SetLobbyPhase(ESFWLobbyPhase::Waiting);
	}
}

void ASFW_LobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer && !NewPlayer->GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("PostLogin: %s has no pawn, calling RestartPlayer"),
			*GetNameSafe(NewPlayer));
		RestartPlayer(NewPlayer);
	}

	UE_LOG(LogTemp, Warning, TEXT("PostLogin: %s pawn after restart = %s"),
		*GetNameSafe(NewPlayer), *GetNameSafe(NewPlayer ? NewPlayer->GetPawn() : nullptr));

	if (ASFW_LobbyGameState* LGS = GetGameState<ASFW_LobbyGameState>())
	{
		if (ASFW_PlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ASFW_PlayerState>() : nullptr)
		{
			if (PS->bIsReady) { PS->SetIsReady(false); }
			LGS->RegisterPlayerState(PS);
			LGS->BroadcastRoster();
		}
	}
}

void ASFW_LobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (ASFW_LobbyGameState* LGS = GetGameState<ASFW_LobbyGameState>())
	{
		if (ASFW_PlayerState* PS = Exiting ? Exiting->GetPlayerState<ASFW_PlayerState>() : nullptr)
		{
			LGS->UnregisterPlayerState(PS);
		}
		LGS->BroadcastRoster();
	}
}
