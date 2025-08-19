// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/SFW_GameState.h"
#include "Net/UnrealNetwork.h"

void ASFW_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_GameState, bRoundActive);
	DOREPLIFETIME(ASFW_GameState, AnomalyAggression);
}
