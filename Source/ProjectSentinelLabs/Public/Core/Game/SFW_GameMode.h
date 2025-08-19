// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SFW_GameMode.generated.h"

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
	
};
