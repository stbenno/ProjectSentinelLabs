// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SFW_GameMode.generated.h"


class USFW_AgentCatalog;
/**
 * 
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_GameMode : public AGameMode
{
	GENERATED_BODY()


	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable)
	void StartRound();

	UFUNCTION(BlueprintCallable)
	void EndRound(bool bSuccess);

	UFUNCTION(BlueprintCallable)
	void FailRound(bool bSuccess);

protected:
	UPROPERTY()
	class ASFW_AnomalyController* AnomalyController = nullptr;

	// --- NEW: provide the same catalog in the match so clients can resolve icons/meshes ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Agents")
	TObjectPtr<USFW_AgentCatalog> AgentCatalog = nullptr;

	// --- NEW: rehydrate players’ selections on join ---
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// --- NEW END ---
	
};
