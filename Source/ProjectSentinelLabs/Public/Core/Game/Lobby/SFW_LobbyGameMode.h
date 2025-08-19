// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SFW_LobbyGameMode.generated.h"


class ASFW_LobbyGameState;
class ASFW_PlayerState;
/**
 * 
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_LobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASFW_LobbyGameMode();

	// AGameModeBase
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
};
