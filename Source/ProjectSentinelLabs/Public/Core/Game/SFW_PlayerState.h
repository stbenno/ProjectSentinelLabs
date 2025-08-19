// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SFW_PlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterIDChanged, FName, NewCharacterID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVariantIDChanged, FName, NewVariantID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyChanged, bool, bNewIsReady);

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
	FName SelectedCharacterID;

	/** Variant/skin identifier (e.g., Default, Winter). */
	UPROPERTY(ReplicatedUsing = OnRep_SelectedVariantID, BlueprintReadOnly, Category = "Appearance")
	FName SelectedVariantID;

	/** Ready state in the lobby. */
	UPROPERTY(ReplicatedUsing = OnRep_IsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady;

	/** Broadcast when SelectedCharacterID changes (client & server). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterIDChanged OnSelectedCharacterIDChanged;

	/** Broadcast when SelectedVariantID changes (client & server). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnVariantIDChanged OnSelectedVariantIDChanged;

	/** Broadcast when bIsReady changes (client & server). */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnReadyChanged OnReadyChanged;

	/** Authority-only: set character + variant together. */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void SetSelectedCharacterAndVariant(const FName& InCharacterID, const FName& InVariantID);

	/** Authority-only: set ready state. */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void SetIsReady(bool bNewReady);

	/** Authority-only: reset ready state when returning to lobby; keep selection. */
	UFUNCTION(BlueprintAuthorityOnly, Category = "Lobby")
	void ResetForLobby();

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_SelectedCharacterID();

	UFUNCTION()
	void OnRep_SelectedVariantID();

	UFUNCTION()
	void OnRep_IsReady();

	/** Notify listeners on appearance change (both ID + variant). */
	void NotifyAppearanceChanged();
};
