// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/Actors/SFW_EquippableBase.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Core/Components/SFW_EquipmentManagerComponent.h"

ASFW_EquippableBase::ASFW_EquippableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetIsReplicated(true);
}

void ASFW_EquippableBase::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* Phys = GetPhysicsComponent())
	{
		InitialPhysicsRelativeTransform = Phys->GetRelativeTransform();
		bHasCachedPhysicsRelativeTransform = true;
	}
}

void ASFW_EquippableBase::OnEquipped(ACharacter* NewOwnerChar)
{
	if (NewOwnerChar)
	{
		SetOwner(NewOwnerChar);
	}

	AttachToCharacter(NewOwnerChar, GetAttachSocketName());

	if (UPrimitiveComponent* Phys = GetPhysicsComponent())
	{
		Phys->SetSimulatePhysics(false);
		Phys->SetEnableGravity(false);
		Phys->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Make sure the visual/physics component is back in its default pose
		if (bHasCachedPhysicsRelativeTransform)
		{
			if (Phys != GetRootComponent())
			{
				Phys->AttachToComponent(
					GetRootComponent(),
					FAttachmentTransformRules::KeepRelativeTransform);
			}

			Phys->SetRelativeTransform(InitialPhysicsRelativeTransform);
		}
	}

	SetActorHiddenInGame(false);
}

void ASFW_EquippableBase::OnUnequipped()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	DetachFromCharacter();
}

void ASFW_EquippableBase::OnDropped(const FVector& DropLocation, const FVector& TossVelocity)
{
	DetachFromCharacter();

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorLocation(DropLocation);

	if (UPrimitiveComponent* Phys = GetPhysicsComponent())
	{
		Phys->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Phys->SetCollisionResponseToAllChannels(ECR_Block);
		Phys->SetSimulatePhysics(true);
		Phys->SetEnableGravity(true);
		Phys->SetPhysicsLinearVelocity(TossVelocity, true);
	}
}

void ASFW_EquippableBase::AttachToCharacter(ACharacter* Char, FName Socket)
{
	if (!Char) return;

	if (USkeletalMeshComponent* CMesh = Char->GetMesh())
	{
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(CMesh, Rules, Socket);
	}
}

void ASFW_EquippableBase::DetachFromCharacter()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

UPrimitiveComponent* ASFW_EquippableBase::GetPhysicsComponent() const
{
	// Default: root
	return Cast<UPrimitiveComponent>(GetRootComponent());
}

FText ASFW_EquippableBase::GetPromptText_Implementation() const
{
	return FText::FromString(TEXT("Pick up item"));
}

void ASFW_EquippableBase::Interact_Implementation(AController* InstigatorController)
{
	if (!InstigatorController || !HasAuthority())
	{
		return;
	}

	ACharacter* Char = Cast<ACharacter>(InstigatorController->GetPawn());
	if (!Char) return;

	USFW_EquipmentManagerComponent* Equip =
		Char->FindComponentByClass<USFW_EquipmentManagerComponent>();
	if (!Equip) return;

	if (UPrimitiveComponent* Phys = GetPhysicsComponent())
	{
		Phys->SetSimulatePhysics(false);
		Phys->SetEnableGravity(false);
		Phys->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetOwner(Char);

	// Puts this instance into inventory and auto-equips if hand empty
	Equip->Server_AddItemToInventory(this, /*bAutoEquipIfHandEmpty=*/true);
}
