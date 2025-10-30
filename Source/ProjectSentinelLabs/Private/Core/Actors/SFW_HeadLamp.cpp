// Fill out your copyright notice in the Description page of Project Settings.


// SFW_HeadLamp.cpp
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

	// Visible mesh for the headlamp
	HeadlampMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadlampMesh"));

	// Safe parent selection
	if (USceneComponent* Parent = GetRootComponent())
	{
		HeadlampMesh->SetupAttachment(Parent);
	}
	else
	{
		// If base didn’t set a root yet, make this the root
		SetRootComponent(HeadlampMesh);
	}

	HeadlampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadlampMesh->SetCastShadow(false);
	HeadlampMesh->SetIsReplicated(true);
	HeadlampMesh->SetVisibility(false);

	// Light attached to the headlamp mesh
	Lamp = CreateDefaultSubobject<USpotLightComponent>(TEXT("Lamp"));
	Lamp->SetupAttachment(HeadlampMesh);
	Lamp->SetMobility(EComponentMobility::Movable);
	Lamp->bUseInverseSquaredFalloff = false;
	Lamp->SetAttenuationRadius(1800.f);
	Lamp->SetInnerConeAngle(24.f);
	Lamp->SetOuterConeAngle(36.f);
	Lamp->SetIntensity(0.f);
	Lamp->SetVisibility(false);
}

void ASFW_HeadLamp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_HeadLamp, bLampEnabled);
}

void ASFW_HeadLamp::OnEquipped(ACharacter* NewOwnerChar)
{
	Super::OnEquipped(NewOwnerChar);

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(NewOwnerChar->GetMesh(), Rules, HeadSocketName);

	// make sure proxies aren’t hidden
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
	HeadlampMesh->SetVisibility(true, true);

	HeadlampMesh->SetRelativeLocation(AttachOffset);
	HeadlampMesh->SetRelativeRotation(AttachRotation);
	HeadlampMesh->SetRelativeScale3D(AttachScale);

	ApplyLightState();      // sets Lamp visibility/intensity on each machine
}

void ASFW_HeadLamp::OnUnequipped()
{
	if (Lamp)
	{
		Lamp->SetVisibility(false);
		Lamp->SetIntensity(0.f);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	Super::OnUnequipped();
}

void ASFW_HeadLamp::ToggleLamp()
{
	SetLampEnabled(!bLampEnabled);
}

void ASFW_HeadLamp::SetLampEnabled(bool bEnabled)
{
	if (!HasAuthority())
	{
		Server_SetLampEnabled(bEnabled);
		return;
	}

	bLampEnabled = bEnabled;
	ApplyLightState();
	Multicast_PlayToggleSFX(bLampEnabled);
}

void ASFW_HeadLamp::Server_SetLampEnabled_Implementation(bool bEnabled)
{
	bLampEnabled = bEnabled;
	ApplyLightState();
	Multicast_PlayToggleSFX(bLampEnabled);
}

void ASFW_HeadLamp::OnRep_LampEnabled()
{
	const bool bOn = bLampEnabled;
	Lamp->SetVisibility(bOn);
	Lamp->SetIntensity(bOn ? 40000.f : 0.f);   // bright for testing; tune later

	Lamp->SetVisibility(bLampEnabled);

	ApplyLightState();
	if (USoundBase* S = (bLampEnabled ? ToggleOnSFX : ToggleOffSFX))
	{
		UGameplayStatics::PlaySoundAtLocation(this, S, GetActorLocation());
	}
}

void ASFW_HeadLamp::ApplyLightState()
{
	const bool bVis = bLampEnabled;
	if (Lamp)
	{
		Lamp->SetVisibility(bVis);
		Lamp->SetIntensity(bVis ? OnIntensity : 0.f);
		Lamp->SetInnerConeAngle(InnerCone);
		Lamp->SetOuterConeAngle(OuterCone);
		Lamp->SetAttenuationRadius(AttenuationRadius);
	}
}

void ASFW_HeadLamp::Multicast_PlayToggleSFX_Implementation(bool bNewEnabled)
{
	USoundBase* S = bNewEnabled ? ToggleOnSFX : ToggleOffSFX;
	if (S) UGameplayStatics::PlaySoundAtLocation(this, S, GetActorLocation());
}

