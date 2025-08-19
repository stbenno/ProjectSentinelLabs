// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Game/SFW_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

ASFW_PlayerState::ASFW_PlayerState()
{
	// Defaults (safe fallbacks; override from save/UI on join)
	SelectedCharacterID = NAME_None;                  // e.g., "Sentinel.AgentA"
	SelectedVariantID = FName(TEXT("Default"));
	bIsReady = false;
}

void ASFW_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFW_PlayerState, SelectedCharacterID);
	DOREPLIFETIME(ASFW_PlayerState, SelectedVariantID);
	DOREPLIFETIME(ASFW_PlayerState, bIsReady);
}

void ASFW_PlayerState::SetSelectedCharacterAndVariant(const FName& InCharacterID, const FName& InVariantID)
{
	check(HasAuthority());

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
		// Server-side bookkeeping hook if needed.
	}
}

void ASFW_PlayerState::SetIsReady(bool bNewReady)
{
	check(HasAuthority());

	if (bIsReady != bNewReady)
	{
		bIsReady = bNewReady;
		OnRep_IsReady(); // server-side immediate notification
	}
}

void ASFW_PlayerState::ResetForLobby()
{
	check(HasAuthority());

	if (bIsReady != false)
	{
		bIsReady = false;
		OnRep_IsReady();
	}

	// Keep SelectedCharacterID/SelectedVariantID as-is to persist preference between rounds.
}

void ASFW_PlayerState::OnRep_SelectedCharacterID()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s CharacterID -> %s"), *GetPlayerName(), *SelectedCharacterID.ToString());
#endif
	if (OnSelectedCharacterIDChanged.IsBound())
	{
		OnSelectedCharacterIDChanged.Broadcast(SelectedCharacterID);
	}
	NotifyAppearanceChanged();
}

void ASFW_PlayerState::OnRep_SelectedVariantID()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s VariantID -> %s"), *GetPlayerName(), *SelectedVariantID.ToString());
#endif
	if (OnSelectedVariantIDChanged.IsBound())
	{
		OnSelectedVariantIDChanged.Broadcast(SelectedVariantID);
	}
	NotifyAppearanceChanged();
}

void ASFW_PlayerState::OnRep_IsReady()
{
#if !UE_BUILD_SHIPPING
	// UE_LOG(LogTemp, Verbose, TEXT("PS %s Ready -> %s"), *GetPlayerName(), bIsReady ? TEXT("true") : TEXT("false"));
#endif
	if (OnReadyChanged.IsBound())
	{
		OnReadyChanged.Broadcast(bIsReady);
	}
}

void ASFW_PlayerState::NotifyAppearanceChanged()
{
	// Central place to react to character/variant changes.
	// Character can bind to delegates or poll the PlayerState on Possess/BeginPlay
	// to apply meshes/anim based on SelectedCharacterID/SelectedVariantID.
	// Intentionally no direct pawn calls here to avoid tight coupling.
}