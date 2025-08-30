// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Actors/SFW_WorldPickup.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Core/Actors/Interface/SFW_InteractableInterface.h"
#include "Core/Components/SFW_EquipmentManagerComponent.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "Core/Actors/SFW_HeadLamp.h"

ASFW_WorldPickup::ASFW_WorldPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetSimulatePhysics(true);
	PickupMesh->SetIsReplicated(true);

	UseBox = CreateDefaultSubobject<UBoxComponent>(TEXT("UseBox"));
	UseBox->SetupAttachment(PickupMesh);
	UseBox->SetBoxExtent(FVector(40.f));
	UseBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UseBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	UseBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ItemDisplayName = FText::FromString(TEXT("Headlamp"));
}

FText ASFW_WorldPickup::GetPromptText_Implementation() const
{
	return FText::Format(FText::FromString(TEXT("Pick up {0}")), ItemDisplayName);
}

void ASFW_WorldPickup::Interact_Implementation(AController* InstigatorController)
{
	if (!InstigatorController) return;

	// Only the server handles giving items
	if (HasAuthority())
	{
		Server_GiveTo(InstigatorController);
	}
}

void ASFW_WorldPickup::Server_GiveTo_Implementation(AController* InstigatorController)
{
	if (!ItemClass) return;

	ACharacter* Char = InstigatorController ? Cast<ACharacter>(InstigatorController->GetPawn()) : nullptr;
	if (!Char) return;

	USFW_EquipmentManagerComponent* Equip = Char->FindComponentByClass<USFW_EquipmentManagerComponent>();
	if (!Equip) return;

	// Headlamp case
	if (ItemClass->IsChildOf(ASFW_HeadLamp::StaticClass()))
	{
		ASFW_HeadLamp* Lamp = GetWorld()->SpawnActor<ASFW_HeadLamp>(ItemClass, GetActorTransform());
		if (!Lamp) return;

		Equip->SetHeadLamp(Lamp);
		Lamp->OnEquipped(Char);
		Destroy();   // remove the pickup from the world
		return;
	}

	// (future: handheld item path)
}