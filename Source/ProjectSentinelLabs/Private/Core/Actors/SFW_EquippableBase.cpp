// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/Actors/SFW_EquippableBase.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

ASFW_EquippableBase::ASFW_EquippableBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetIsReplicated(true);
}

void ASFW_EquippableBase::OnEquipped(ACharacter* NewOwnerChar)
{
	AttachToCharacter(NewOwnerChar, GetAttachSocketName());
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
}

void ASFW_EquippableBase::OnUnequipped()
{
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	DetachFromCharacter();
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

// GrantsRadioComms() default implementation lives in header (returns false)
