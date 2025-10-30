// Fill out your copyright notice in the Description page of Project Settings.

// SFW_Flashlight.h

#pragma once

#include "CoreMinimal.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "PlayerCharacter/Animation/SFW_EquipmentTypes.h"
#include "SFW_Flashlight.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class USoundBase;

/** Replicated handheld flashlight. One spotlight on the flashlight actor, toggled on Use. */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_Flashlight : public ASFW_EquippableBase
{
	GENERATED_BODY()

public:
	ASFW_Flashlight();

	// Equippable overrides
	virtual void OnEquipped(ACharacter* NewOwnerChar) override;
	virtual void OnUnequipped() override;
	virtual void PrimaryUse() override;

	// Toggle API
	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void ToggleLight();

	UFUNCTION(BlueprintCallable, Category = "Flashlight")
	void SetLightEnabled(bool bEnable);

	// Queries
	UFUNCTION(BlueprintPure, Category = "Flashlight")
	bool IsOn() const { return bIsOn; }

	UFUNCTION(BlueprintPure, Category = "Flashlight")
	EHeldItemType GetItemType() const { return EHeldItemType::Flashlight; }

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<UStaticMeshComponent> FlashlightMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flashlight")
	TObjectPtr<USpotLightComponent> Spot;

	// IES Profile
	// Light | Photometric
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	TObjectPtr<UTextureLightProfile> IESProfile = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	bool bUseIESBrightness = true;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float IESBrightnessScale = 1.0f;

	// Optional: use real light units
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	ELightUnits IntensityUnits = ELightUnits::Lumens;

	// Recommended game-ish defaults
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float OnIntensity = 900.f;        // ~900 lm flashlight beam (start here)
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float AttenuationRadius = 2200.f; // falloff distance
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float InnerCone = 12.f;
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float OuterCone = 20.f;

	// Replicated state
	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	// Tuning
	

	// SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|SFX")
	TObjectPtr<USoundBase> ToggleOnSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flashlight|SFX")
	TObjectPtr<USoundBase> ToggleOffSFX = nullptr;

	// RepNotifies
	UFUNCTION()
	void OnRep_IsOn();

	// Server authority
	UFUNCTION(Server, Reliable)
	void Server_SetLightEnabled(bool bEnable);

	// Cosmetic sync
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayToggleSFX(bool bEnable);

private:
	void ApplyLightState();
	USkeletalMeshComponent* ResolveOwnerMesh(ACharacter* InChar) const;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
