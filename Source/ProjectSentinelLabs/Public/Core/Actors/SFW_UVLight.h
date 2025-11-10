// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "PlayerCharacter/Animation/SFW_EquipmentTypes.h"
#include "SFW_UVLight.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class USoundBase;
class UTextureLightProfile;
class UPrimitiveComponent;

/** Handheld UV light. Same family as Flashlight, different beam. */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_UVLight : public ASFW_EquippableBase
{
	GENERATED_BODY()

public:
	ASFW_UVLight();

	// Equippable overrides
	virtual void OnEquipped(ACharacter* NewOwnerChar) override;
	virtual void OnUnequipped() override;
	virtual void PrimaryUse() override;

	// Anim type override
	virtual EHeldItemType GetAnimHeldType_Implementation() const override;
	

	// Toggle API
	UFUNCTION(BlueprintCallable, Category = "UVLight")
	void ToggleLight();

	UFUNCTION(BlueprintCallable, Category = "UVLight")
	void SetLightEnabled(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "UVLight")
	bool IsOn() const { return bIsOn; }

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UVLight")
	TObjectPtr<UStaticMeshComponent> UVMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UVLight")
	TObjectPtr<USpotLightComponent> Spot;

	// Photometric settings
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	TObjectPtr<UTextureLightProfile> IESProfile = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	bool bUseIESBrightness = true;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float IESBrightnessScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	ELightUnits IntensityUnits = ELightUnits::Lumens;

	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float OnIntensity = 500.f;        // dimmer than main flashlight
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float AttenuationRadius = 1800.f;
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float InnerCone = 10.f;
	UPROPERTY(EditDefaultsOnly, Category = "Light|Photometric")
	float OuterCone = 18.f;

	// Replicated state
	UPROPERTY(ReplicatedUsing = OnRep_IsOn)
	bool bIsOn = false;

	// SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UVLight|SFX")
	TObjectPtr<USoundBase> ToggleOnSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UVLight|SFX")
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

	// Physics body
	virtual UPrimitiveComponent* GetPhysicsComponent() const override;

	// Attach socket (tool grip)
	//virtual FName GetAttachSocketName() const override
	//{
		//return TEXT("hand_R_Tool");
	//}

private:
	void ApplyLightState();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

