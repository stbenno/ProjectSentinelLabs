// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"          // <-- switched from GameStateBase to GameState
#include "SFW_LobbyGameState.generated.h"

class ASFW_PlayerState;

/** Lobby phases */
UENUM(BlueprintType)
enum class ESFWLobbyPhase : uint8
{
	Waiting  UMETA(DisplayName = "Waiting"),
	Locked   UMETA(DisplayName = "Locked"),
	InMatch  UMETA(DisplayName = "InMatch")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFWOnRosterUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFWOnLobbyPhaseChanged, ESFWLobbyPhase, NewPhase);

/**
 * GameState for the lobby. Tracks phase and rebroadcasts roster changes.
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_LobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASFW_LobbyGameState();

	/** Current lobby phase (replicated). */
	UPROPERTY(ReplicatedUsing = OnRep_LobbyPhase, BlueprintReadOnly, Category = "Lobby")
	ESFWLobbyPhase LobbyPhase;

	/** Fired whenever the roster should refresh (join/leave or PS field change). */
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FSFWOnRosterUpdated OnRosterUpdated;

	/** Fired when LobbyPhase changes (client & server). */
	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FSFWOnLobbyPhaseChanged OnLobbyPhaseChanged;

	/** Authority-only: set the lobby phase. */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void SetLobbyPhase(ESFWLobbyPhase NewPhase);

	/** Register/unregister a PlayerState so we can listen for its changes (call from GameMode PostLogin/Logout). */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void RegisterPlayerState(ASFW_PlayerState* PS);

	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void UnregisterPlayerState(ASFW_PlayerState* PS);

	/** Manually trigger a roster refresh broadcast. */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void BroadcastRoster();

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** RepNotify for LobbyPhase. */
	UFUNCTION()
	void OnRep_LobbyPhase();

	/** Handlers bound to PlayerState dynamic delegates. */
	UFUNCTION()
	void HandlePSCharacterIDChanged(FName NewCharacterID);

	UFUNCTION()
	void HandlePSVariantIDChanged(FName NewVariantID);

	UFUNCTION()
	void HandlePSReadyChanged(bool bNewIsReady);
};