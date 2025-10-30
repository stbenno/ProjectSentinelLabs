// Fill out your copyright notice in the Description page of Project Settings.

// SFW_Flashlight.cpp

#include "Core/Actors/SFW_Flashlight.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASFW_Flashlight::ASFW_Flashlight()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Visible mesh
	FlashlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlashlightMesh"));
	FlashlightMesh->SetupAttachment(GetMesh());                 // GetMesh() is the root from EquippableBase
	FlashlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlashlightMesh->SetCastShadow(true);
	FlashlightMesh->SetIsReplicated(true);
	FlashlightMesh->SetVisibility(true, true);

	// Light (no function calls that might assume registration)
	Spot = CreateDefaultSubobject<USpotLightComponent>(TEXT("Spot"));
	Spot->SetupAttachment(FlashlightMesh);
	Spot->Mobility = EComponentMobility::Movable;
	Spot->bUseInverseSquaredFalloff = false;
	Spot->AttenuationRadius = 1400.f;
	Spot->Intensity = 0.f;                 // start OFF
	Spot->InnerConeAngle = 16.f;
	Spot->OuterConeAngle = 28.f;
	Spot->SetVisibility(false, true);
}

void ASFW_Flashlight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_Flashlight, bIsOn);
}

USkeletalMeshComponent* ASFW_Flashlight::ResolveOwnerMesh(ACharacter* InChar) const
{
	return InChar ? InChar->GetMesh() : nullptr; // 3P mesh for everyone
}

void ASFW_Flashlight::OnEquipped(ACharacter* NewOwnerChar)
{
	if (USkeletalMeshComponent* OwnerMesh = ResolveOwnerMesh(NewOwnerChar))
	{
		FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(OwnerMesh, Rules, FName(TEXT("hand_R_Tool")));
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);

	ApplyLightState();
	Super::OnEquipped(NewOwnerChar);
}

void ASFW_Flashlight::OnUnequipped()
{
	if (Spot)
	{
		Spot->SetVisibility(false);
		Spot->SetIntensity(0.f);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	Super::OnUnequipped();
}

void ASFW_Flashlight::PrimaryUse()
{
	UE_LOG(LogTemp, Warning, TEXT("Flashlight::PrimaryUse (On=%d)"), bIsOn ? 1 : 0);
	ToggleLight();
}

void ASFW_Flashlight::ToggleLight()
{
	UE_LOG(LogTemp, Warning, TEXT("Flashlight::ToggleLight -> %s"), bIsOn ? TEXT("OFF") : TEXT("ON"));
	SetLightEnabled(!bIsOn);
}

void ASFW_Flashlight::SetLightEnabled(bool bEnable)
{
	if (!HasAuthority())
	{
		Server_SetLightEnabled(bEnable);
		return;
	}

	bIsOn = bEnable;
	ApplyLightState();
	Multicast_PlayToggleSFX(bIsOn);
}

void ASFW_Flashlight::Server_SetLightEnabled_Implementation(bool bEnable)
{
	bIsOn = bEnable;
	ApplyLightState();
	Multicast_PlayToggleSFX(bIsOn);
}

void ASFW_Flashlight::OnRep_IsOn()
{
	ApplyLightState();
}

void ASFW_Flashlight::ApplyLightState()
{
	const bool bVis = bIsOn;
	if (Spot)
	{
		Spot->SetVisibility(bVis);
		Spot->SetIntensity(bVis ? OnIntensity : 0.f);
		Spot->SetInnerConeAngle(InnerCone);
		Spot->SetOuterConeAngle(OuterCone);
		Spot->SetAttenuationRadius(AttenuationRadius);
	}
}

void ASFW_Flashlight::Multicast_PlayToggleSFX_Implementation(bool bEnable)
{
	USoundBase* SFX = bEnable ? ToggleOnSFX : ToggleOffSFX;
	if (SFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SFX, GetActorLocation());
	}
}
