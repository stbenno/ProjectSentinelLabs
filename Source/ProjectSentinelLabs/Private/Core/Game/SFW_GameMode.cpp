// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/SFW_GameMode.h"
#include "Core/AnomalySystems/SFW_AnomalyController.h"
#include "Core/Game/SFW_GameState.h"
#include "EngineUtils.h"

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
