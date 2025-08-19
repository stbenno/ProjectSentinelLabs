// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SFW_GameState.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_GameState : public AGameState
{
	GENERATED_BODY()

public:

	UPROPERTY(Replicated, BlueprintReadonly)
	float AnomalyAggression = 0.f;


	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Round")
	bool bRoundActive = false;
	
};
