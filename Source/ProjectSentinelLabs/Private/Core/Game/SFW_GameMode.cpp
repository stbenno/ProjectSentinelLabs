// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/SFW_GameMode.h"
#include "Core/AnomalySystems/SFW_AnomalyController.h"
#include "Core/Game/SFW_GameState.h"
#include "EngineUtils.h"

#include "Core/Game/SFW_GameInstance.h"
#include "Core/Game/SFW_PlayerState.h"
#include "PlayerCharacter/Data/SFW_AgentCatalog.h"

void ASFW_GameMode::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		StartRound();
	}
	
}

void ASFW_GameMode::StartRound()
{
	if (!HasAuthority()) return;
	
	//Reset State
	if (ASFW_GameState* GS = GetGameState<ASFW_GameState>())
	{
		GS->bRoundActive = true;
		GS->AnomalyAggression = 0.f;
	}

	//Clean any previous controller (hot reload/restart 
	if (IsValid(AnomalyController) && !AnomalyController->IsActorBeingDestroyed())
	{
		AnomalyController->Destroy();
		AnomalyController = nullptr;
	}

	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AnomalyController = GetWorld()->SpawnActor<ASFW_AnomalyController>(ASFW_AnomalyController::StaticClass(), FTransform::Identity, Params);

	UE_LOG(LogTemp, Warning, TEXT("StartRound: AnomalyController Spawned."));
}

void ASFW_GameMode::EndRound(bool bSuccess)
{
	
	
	if (!HasAuthority()) return;

	if (ASFW_GameState* GS = GetGameState<ASFW_GameState>())
	{
		GS->bRoundActive = false;
	}

	// Stop anomaly ticking
	if (IsValid(AnomalyController) && !AnomalyController->IsActorBeingDestroyed())
	{
		AnomalyController->Destroy();
		AnomalyController = nullptr;
	}

	UE_LOG(LogTemp, Warning, TEXT("Round ended. Success=%d"), bSuccess);

	// TODO: cleanup / transition (reload map, go to lobby, etc.)
}

void ASFW_GameMode::FailRound(bool bSuccess)
{
	EndRound(false);
}

void ASFW_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!HasAuthority() || !NewPlayer) return;

	// Give the player the catalog so client UIs/pawns can resolve data
	if (ASFW_PlayerState* PS = NewPlayer->GetPlayerState<ASFW_PlayerState>())
	{
		PS->AgentCatalog = AgentCatalog; // replicated

		// Pull their pre-match selection from GameInstance (cached in the lobby before travel)
		if (USFW_GameInstance* GI = GetGameInstance<USFW_GameInstance>())
		{
			const FString Key = USFW_GameInstance::MakePlayerKey(PS);
			FSFWPreMatchData Saved;
			if (GI->GetPreMatchData(Key, Saved))
			{
				// Set both character and variant (authority)
				// First set by ID so CharacterIndex maps correctly if present in the catalog
				PS->ServerSetCharacterByID(Saved.CharacterID);
				// Then ensure variant matches (keep current SelectedCharacterID in case mapping changed)
				PS->SetSelectedCharacterAndVariant(PS->SelectedCharacterID, Saved.VariantID);
			}
		}
	}
}
