// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/SFW_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "PlayerCharacter/Data/SFW_AgentCatalog.h"   

ASFW_PlayerState::ASFW_PlayerState()
{
	// Defaults set in header
}

void ASFW_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFW_PlayerState, SelectedCharacterID);
	DOREPLIFETIME(ASFW_PlayerState, SelectedVariantID);
	DOREPLIFETIME(ASFW_PlayerState, bIsReady);
	DOREPLIFETIME(ASFW_PlayerState, bIsHost); // NEW

	// --- NEW ---
	DOREPLIFETIME(ASFW_PlayerState, AgentCatalog);
	DOREPLIFETIME(ASFW_PlayerState, CharacterIndex);
	// --- NEW END ---
}

void ASFW_PlayerState::SetSelectedCharacterAndVariant(const FName& InCharacterID, const FName& InVariantID)
{
	if (!HasAuthority()) return;

	bool bChanged = false;

	if (SelectedCharacterID != InCharacterID)
	{
		SelectedCharacterID = InCharacterID;
		OnRep_SelectedCharacterID(); // server-side immediate notification
		bChanged = true;
	}

	if (SelectedVariantID != InVariantID)
	{
		SelectedVariantID = InVariantID;
		OnRep_SelectedVariantID(); // server-side immediate notification
		bChanged = true;
	}

	if (bChanged)
	{
		// hook if needed
	}
}

void ASFW_PlayerState::SetIsReady(bool bNewReady)
{
	if (!HasAuthority()) return;
	if (bIsReady == bNewReady) return;

	bIsReady = bNewReady;
	OnRep_IsReady(); // server-side immediate notification
}

void ASFW_PlayerState::ResetForLobby()
{
	if (!HasAuthority()) return;

	if (bIsReady)
	{
		bIsReady = false;
		OnRep_IsReady();
	}

	// Keep SelectedCharacterID/SelectedVariantID to persist preference
}

void ASFW_PlayerState::ServerSetIsHost(bool bNewIsHost)
{
	check(HasAuthority());
	if (bIsHost == bNewIsHost) return;

	bIsHost = bNewIsHost;
	OnRep_IsHost(); // server-side immediate notify for local UI hooks
}

void ASFW_PlayerState::OnRep_SelectedCharacterID()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s CharacterID -> %s"), *GetPlayerName(), *SelectedCharacterID.ToString());
#endif
	OnSelectedCharacterIDChanged.Broadcast(SelectedCharacterID);
	NotifyAppearanceChanged();
}

void ASFW_PlayerState::OnRep_SelectedVariantID()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s VariantID -> %s"), *GetPlayerName(), *SelectedVariantID.ToString());
#endif
	OnSelectedVariantIDChanged.Broadcast(SelectedVariantID);
	NotifyAppearanceChanged();
}

void ASFW_PlayerState::OnRep_IsReady()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s Ready -> %s"), *GetPlayerName(), bIsReady ? TEXT("true") : TEXT("false"));
#endif
	OnReadyChanged.Broadcast(bIsReady);
}

void ASFW_PlayerState::OnRep_IsHost()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s Host -> %s"), *GetPlayerName(), bIsHost ? TEXT("true") : TEXT("false"));
#endif
	OnHostFlagChanged.Broadcast(bIsHost);
}

// --- NEW ---
void ASFW_PlayerState::OnRep_CharacterIndex()
{
	// When index changes on clients, ensure SelectedCharacterID reflects it if we have a valid catalog.
	ApplyIndexToSelectedID();
}
// --- NEW END ---

void ASFW_PlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();
	OnPlayerNameChanged.Broadcast(GetPlayerName());
}

void ASFW_PlayerState::NotifyAppearanceChanged()
{
	// Central place to react to character/variant changes.
	// Avoid tight coupling to Pawn/Character; let them query PS when needed.
}

// =====================
// NEW: Helpers & RPCs
// =====================

int32 ASFW_PlayerState::GetAgentCount() const
{
	return AgentCatalog ? AgentCatalog->Agents.Num() : 0;
}

void ASFW_PlayerState::NormalizeIndex()
{
	const int32 Count = GetAgentCount();
	if (Count <= 0)
	{
		CharacterIndex = 0;
		return;
	}
	CharacterIndex = (CharacterIndex % Count + Count) % Count; // wrap-safe
}

void ASFW_PlayerState::ApplyIndexToSelectedID()
{
	// Only map index -> ID when catalog is valid; otherwise leave SelectedCharacterID as-is.
	if (!AgentCatalog) return;

	const int32 Count = GetAgentCount();
	if (Count <= 0) return;

	if (!AgentCatalog->Agents.IsValidIndex(CharacterIndex)) return;

	const FName NewID = AgentCatalog->Agents[CharacterIndex].AgentID;
	if (SelectedCharacterID != NewID)
	{
		SelectedCharacterID = NewID;

		// Fire local notify when this happens via index replication
		OnRep_SelectedCharacterID();
	}
}

int32 ASFW_PlayerState::FindIndexByAgentID(FName InCharacterID) const
{
	if (!AgentCatalog) return INDEX_NONE;
	const int32 Count = GetAgentCount();
	for (int32 i = 0; i < Count; ++i)
	{
		if (AgentCatalog->Agents[i].AgentID == InCharacterID)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void ASFW_PlayerState::ServerSetCharacterIndex_Implementation(int32 NewIndex)
{
	if (!HasAuthority()) return;

	CharacterIndex = NewIndex;
	NormalizeIndex();

	// Map index -> ID (authoritative) and notify
	ApplyIndexToSelectedID();

	// SelectedVariantID left unchanged here
}

void ASFW_PlayerState::ServerCycleCharacter_Implementation(int32 Direction)
{
	if (!HasAuthority()) return;

	const int32 Count = GetAgentCount();
	if (Count <= 0) return;

	CharacterIndex += (Direction >= 0 ? 1 : -1);
	NormalizeIndex();
	ApplyIndexToSelectedID();
}

void ASFW_PlayerState::ServerSetCharacterByID_Implementation(FName InCharacterID)
{
	if (!HasAuthority()) return;

	// If the catalog knows this ID, sync both index and ID for consistency.
	const int32 Found = FindIndexByAgentID(InCharacterID);
	if (Found != INDEX_NONE)
	{
		CharacterIndex = Found;
		ApplyIndexToSelectedID(); // sets SelectedCharacterID & notifies
		return;
	}

	// Otherwise allow direct set of ID (e.g., catalog not assigned yet).
	if (SelectedCharacterID != InCharacterID)
	{
		SelectedCharacterID = InCharacterID;
		OnRep_SelectedCharacterID();
	}
}
