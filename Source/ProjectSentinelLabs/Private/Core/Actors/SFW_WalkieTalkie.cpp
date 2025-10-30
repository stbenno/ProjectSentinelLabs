// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Actors/SFW_WalkieTalkie.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

ASFW_WalkieTalkie::ASFW_WalkieTalkie()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	// Mesh comes from ASFW_EquippableBase (Mesh is already root)
	// You will assign the actual walkie mesh/SK in the Blueprint child of this C++ class.
	// No extra setup needed here for now.

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ASFW_WalkieTalkie::PrimaryUse()
{
	// This will later map to "force transmit now" or "open radio channel"
	// For now we just log so you can confirm it's wired.
	UE_LOG(LogTemp, Log, TEXT("WalkieTalkie::PrimaryUse() called (push-to-talk placeholder)"));
}


