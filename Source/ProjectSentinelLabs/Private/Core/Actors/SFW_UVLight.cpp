// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Actors/SFW_UVLight.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASFW_UVLight::ASFW_UVLight()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;

	// Visible mesh body
	UVMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UVMesh"));
	UVMesh->SetupAttachment(GetMesh());
	UVMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UVMesh->SetCastShadow(true);
	UVMesh->SetIsReplicated(true);
	UVMesh->SetVisibility(true, true);

	EquipSlot = ESFWEquipSlot::Hand_Light;

	// UV spot
	Spot = CreateDefaultSubobject<USpotLightComponent>(TEXT("UVSpot"));
	Spot->SetupAttachment(UVMesh);
	Spot->Mobility = EComponentMobility::Movable;
	Spot->IntensityUnits = IntensityUnits;
	Spot->bUseInverseSquaredFalloff = false;
	Spot->AttenuationRadius = AttenuationRadius;
	Spot->Intensity = 0.f;
	Spot->InnerConeAngle = InnerCone;
	Spot->OuterConeAngle = OuterCone;
	Spot->SetVisibility(false, true);

	bIsOn = false;
}

void ASFW_UVLight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFW_UVLight, bIsOn);
}

UPrimitiveComponent* ASFW_UVLight::GetPhysicsComponent() const
{
	return UVMesh ? UVMesh : Super::GetPhysicsComponent();
}

void ASFW_UVLight::OnEquipped(ACharacter* NewOwnerChar)
{
	Super::OnEquipped(NewOwnerChar);

	if (UVMesh)
	{
		UVMesh->SetSimulatePhysics(false);
		UVMesh->SetEnableGravity(false);
		UVMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UVMesh->SetVisibility(true, true);
	}

	ApplyLightState();
}

void ASFW_UVLight::OnUnequipped()
{
	if (Spot)
	{
		Spot->SetVisibility(false);
		Spot->SetIntensity(0.f);
	}

	if (UVMesh)
	{
		UVMesh->SetSimulatePhysics(false);
		UVMesh->SetEnableGravity(false);
		UVMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Super::OnUnequipped();
}

void ASFW_UVLight::PrimaryUse()
{
	UE_LOG(LogTemp, Log, TEXT("UVLight::PrimaryUse (On=%d)"), bIsOn ? 1 : 0);
	ToggleLight();
}

EHeldItemType ASFW_UVLight::GetAnimHeldType_Implementation() const
{
	
	return EHeldItemType::UVLight;
	
}

void ASFW_UVLight::ToggleLight()
{
	SetLightEnabled(!bIsOn);
}

void ASFW_UVLight::SetLightEnabled(bool bEnable)
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

void ASFW_UVLight::Server_SetLightEnabled_Implementation(bool bEnable)
{
	bIsOn = bEnable;
	ApplyLightState();
	Multicast_PlayToggleSFX(bIsOn);
}

void ASFW_UVLight::OnRep_IsOn()
{
	ApplyLightState();
}

void ASFW_UVLight::ApplyLightState()
{
	const bool bVis = bIsOn;

	if (Spot)
	{
		Spot->SetVisibility(bVis);
		Spot->SetIntensity(bVis ? OnIntensity : 0.f);
		Spot->SetInnerConeAngle(InnerCone);
		Spot->SetOuterConeAngle(OuterCone);
		Spot->SetAttenuationRadius(AttenuationRadius);

		if (IESProfile)
		{
			Spot->SetIESTexture(IESProfile);
			Spot->bUseIESBrightness = bUseIESBrightness;
			Spot->IESBrightnessScale = IESBrightnessScale;
		}
	}
}

void ASFW_UVLight::Multicast_PlayToggleSFX_Implementation(bool bEnable)
{
	USoundBase* SFX = bEnable ? ToggleOnSFX : ToggleOffSFX;
	if (SFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SFX, GetActorLocation());
	}
}
