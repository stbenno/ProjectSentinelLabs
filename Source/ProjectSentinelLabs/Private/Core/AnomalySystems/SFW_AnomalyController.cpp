// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/AnomalySystems/SFW_AnomalyController.h"
#include "Core/Game/SFW_GameState.h"
#include "Core/Game/SFW_GameMode.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ASFW_AnomalyController::ASFW_AnomalyController()
{
 	
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;  // Manager writes through GameState instead

}

// Called when the game starts or when spawned
void ASFW_AnomalyController::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority()) //server only
	{
		GetWorldTimerManager().SetTimer(
			TickHandle, this, &ASFW_AnomalyController::TickAggro, TickInterval, true);
	}
}

void ASFW_AnomalyController::TickAggro()
{
	if (!HasAuthority()) return;
	
	if (ASFW_GameState* GS = GetWorld()->GetGameState<ASFW_GameState>())
	{
		GS->AnomalyAggression = FMath::Clamp(GS->AnomalyAggression + AggroPerTick, 0.f, CollapseThreshold);

		if (GS->AnomalyAggression >= CollapseThreshold)
		{
			if (ASFW_GameMode* GM = Cast<ASFW_GameMode>(UGameplayStatics::GetGameMode(this)))
			{
				GM->FailRound(false);  //server side fail
			}
		}
	}
}

