// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/Lobby/SFW_LobbyGameState.h"
#include "Core/Game/SFW_PlayerState.h"
#include "Net/UnrealNetwork.h"

ASFW_LobbyGameState::ASFW_LobbyGameState()
{
	LobbyPhase = ESFWLobbyPhase::Waiting;
}

void ASFW_LobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFW_LobbyGameState, LobbyPhase);
}

void ASFW_LobbyGameState::SetLobbyPhase(ESFWLobbyPhase NewPhase)
{
	check(HasAuthority());

	if (LobbyPhase != NewPhase)
	{
		LobbyPhase = NewPhase;
		OnRep_LobbyPhase(); // server-side notification
	}
}

void ASFW_LobbyGameState::OnRep_LobbyPhase()
{
	OnLobbyPhaseChanged.Broadcast(LobbyPhase);
}

void ASFW_LobbyGameState::RegisterPlayerState(ASFW_PlayerState* PS)
{
	if (!HasAuthority() || !IsValid(PS))
	{
		return;
	}

	// Bind to PlayerState dynamic multicast delegates so any change triggers a roster refresh.
	PS->OnSelectedCharacterIDChanged.AddDynamic(this, &ASFW_LobbyGameState::HandlePSCharacterIDChanged);
	PS->OnSelectedVariantIDChanged.AddDynamic(this, &ASFW_LobbyGameState::HandlePSVariantIDChanged);
	PS->OnReadyChanged.AddDynamic(this, &ASFW_LobbyGameState::HandlePSReadyChanged);

	// Notify UI to refresh now that someone joined.
	BroadcastRoster();
}

void ASFW_LobbyGameState::UnregisterPlayerState(ASFW_PlayerState* PS)
{
	if (!HasAuthority() || !IsValid(PS))
	{
		return;
	}

	// Remove our bindings from that PlayerState’s delegates.
	PS->OnSelectedCharacterIDChanged.RemoveDynamic(this, &ASFW_LobbyGameState::HandlePSCharacterIDChanged);
	PS->OnSelectedVariantIDChanged.RemoveDynamic(this, &ASFW_LobbyGameState::HandlePSVariantIDChanged);
	PS->OnReadyChanged.RemoveDynamic(this, &ASFW_LobbyGameState::HandlePSReadyChanged);

	// Notify UI to refresh now that someone left.
	BroadcastRoster();
}

void ASFW_LobbyGameState::BroadcastRoster()
{
	OnRosterUpdated.Broadcast();
}

void ASFW_LobbyGameState::HandlePSCharacterIDChanged(FName /*NewCharacterID*/)
{
	OnRosterUpdated.Broadcast();
}

void ASFW_LobbyGameState::HandlePSVariantIDChanged(FName /*NewVariantID*/)
{
	OnRosterUpdated.Broadcast();
}

void ASFW_LobbyGameState::HandlePSReadyChanged(bool /*bNewIsReady*/)
{
	OnRosterUpdated.Broadcast();
}