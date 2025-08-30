// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Actors/SFW_HeadLamp.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASFW_HeadLamp::ASFW_HeadLamp()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	HeadlampMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadlampMesh"));
	HeadlampMesh->SetupAttachment(GetMesh());
	HeadlampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadlampMesh->SetCastShadow(false);
	HeadlampMesh->SetIsReplicated(true);
	HeadlampMesh->SetVisibility(false);

	Lamp = CreateDefaultSubobject<USpotLightComponent>(TEXT("Lamp"));
	Lamp->SetupAttachment(GetMesh());
	Lamp->SetAttenuationRadius(1200.f);
	Lamp->SetIntensity(3000.f);
	Lamp->SetInnerConeAngle(24.f);
	Lamp->SetOuterConeAngle(36.f);
	Lamp->bUseInverseSquaredFalloff = false;
	Lamp->SetVisibility(false);
}

void ASFW_HeadLamp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_HeadLamp, bLampEnabled);
}

void ASFW_HeadLamp::OnEquipped(ACharacter* NewOwnerChar)
{
	if (NewOwnerChar && NewOwnerChar->GetMesh())
	{
		const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(NewOwnerChar->GetMesh(), Rules, HeadSocketName);
		SetActorRelativeLocation(AttachOffset);
		SetActorRelativeRotation(AttachRotation);
		SetActorRelativeScale3D(AttachScale);
	}

	// Always visible while equipped; only the LIGHT toggles
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
	HeadlampMesh->SetVisibility(true);

	// Default: ON for the whole match unless player toggles off
	SetLampEnabled(true);
}

void ASFW_HeadLamp::OnUnequipped()
{
	SetLampEnabled(false);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ASFW_HeadLamp::OnRep_LampEnabled()
{
	//  Only toggle the spotlight (not the mesh)
	Lamp->SetVisibility(bLampEnabled);

	if (ToggleSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ToggleSound, GetActorLocation());
	}
}

void ASFW_HeadLamp::SetLampEnabled(bool bEnabled)
{
	bLampEnabled = bEnabled;
	OnRep_LampEnabled(); // apply locally; replication updates others
}

void ASFW_HeadLamp::ToggleLamp()
{
	SetLampEnabled(!bLampEnabled);
}

