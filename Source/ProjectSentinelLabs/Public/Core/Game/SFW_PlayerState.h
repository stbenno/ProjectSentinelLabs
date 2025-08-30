// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SFW_PlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterIDChanged, FName, NewCharacterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantIDChanged, FName, NewVariantID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyChanged, bool, bNewIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerNameChanged, const FString&, NewName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostFlagChanged, bool, bNewIsHost);

// --- NEW: forward declare catalog ---
class USFW_AgentCatalog;

/**
 * PlayerState used across Lobby ↔ Match.
 * Holds authoritative character selection and ready state.
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_PlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASFW_PlayerState();

	/** Stable character identifier (e.g., Sentinel.AgentA). */
	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacterID, BlueprintReadOnly, Category = "Appearance")
	FName SelectedCharacterID = NAME_None;

	/** Variant/skin identifier (e.g., Default, Winter). */
	UPROPERTY(ReplicatedUsing = OnRep_SelectedVariantID, BlueprintReadOnly, Category = "Appearance")
	FName SelectedVariantID = FName(TEXT("Default"));

	/** Ready state in the lobby. */
	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	/** NEW: Host flag (server authoritative, replicated). */
	UPROPERTY(ReplicatedUsing = OnRep_IsHost, BlueprintReadOnly, Category = "Lobby")
	bool bIsHost = false;

	// --- NEW: Catalog + index (for left/right cycling & easy scale-up) ---
	/** Replicated reference to the shared Agent Catalog (set by GameMode). */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Appearance")
	TObjectPtr<USFW_AgentCatalog> AgentCatalog = nullptr;

	/** Replicated index into AgentCatalog (0 = Agent A). Drives SelectedCharacterID when catalog is present. */
	UPROPERTY(ReplicatedUsing = OnRep_CharacterIndex, BlueprintReadOnly, Category = "Appearance")
	int32 CharacterIndex = 0;
	// --- NEW END ---

	/** Broadcast when fields change (client & server). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterIDChanged OnSelectedCharacterIDChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnVariantIDChanged OnSelectedVariantIDChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnReadyChanged OnReadyChanged;

	/** Optional: player-name change propagation (useful for UI). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerNameChanged OnPlayerNameChanged;

	/** Optional: host flag changed (for widgets). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHostFlagChanged OnHostFlagChanged;

	/** Authority-only: set character + variant together. */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetSelectedCharacterAndVariant(const FName& InCharacterID, const FName& InVariantID);

	/** Authority-only: set ready state. */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetIsReady(bool bNewReady);

	/** Authority-only: reset ready state when returning to lobby; keep selection. */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void ResetForLobby();

	/** Helper for UI/logic. */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool GetIsHost() const { return bIsHost; }

	/** Server-only setter used by GameMode to promote/demote host. */
	void ServerSetIsHost(bool bNewIsHost);

	// --- NEW: client→server RPCs for selector UI ---
	/** Client→Server: select by absolute index (wrap-safe). */
	UFUNCTION(Server, Reliable,BlueprintCallable, Category = "Appearance")
	void ServerSetCharacterIndex(int32 NewIndex);

	/** Client→Server: cycle -1 / +1. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Appearance")
	void ServerCycleCharacter(int32 Direction);

	/** Client→Server: select by ID (for direct jumps). Falls back to setting SelectedCharacterID if not in catalog. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Appearance")
	void ServerSetCharacterByID(FName InCharacterID);
	// --- NEW END ---

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** RepNotifies */
	UFUNCTION() void OnRep_SelectedCharacterID();
	UFUNCTION() void OnRep_SelectedVariantID();
	UFUNCTION() void OnRep_IsReady();
	UFUNCTION() void OnRep_IsHost();           // NEW

	// --- NEW ---
	UFUNCTION() void OnRep_CharacterIndex();
	// --- NEW END ---

	/** APlayerState name changed (replicated) */
	virtual void OnRep_PlayerName() override;

	/** Notify listeners on appearance change (both ID + variant). */
	void NotifyAppearanceChanged();

private:
	// --- NEW: helpers ---
	int32 GetAgentCount() const;
	void NormalizeIndex();
	void ApplyIndexToSelectedID(); // maps CharacterIndex -> SelectedCharacterID (when catalog is valid)
	int32 FindIndexByAgentID(FName InCharacterID) const;
	// --- NEW END ---
};

