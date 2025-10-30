// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "TimerManager.h"
#include "SFW_EMFDevice.generated.h"

class UStaticMeshComponent;
class UAudioComponent;
class USoundBase;
class UMaterialInstanceDynamic;
class ACharacter;

/**
 * Handheld EMF meter.
 * PrimaryUse() toggles power on the server.
 * EMFLevel (0..5) drives LED brightness.
 * LED[0] stays lit when active as "power on".
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_EMFDevice : public ASFW_EquippableBase
{
	GENERATED_BODY()

public:
	ASFW_EMFDevice();

	virtual void BeginPlay() override;

	// When equipped in hand
	virtual void OnEquipped(ACharacter* NewOwnerChar) override;
	virtual void OnUnequipped() override;

	// Player pressed IA_Use while this is active in hand
	virtual void PrimaryUse() override;

	// ---- Power control ----

	// Blueprint-callable hard set
	UFUNCTION(BlueprintCallable, Category = "EMF")
	void SetActive(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "EMF")
	bool IsActive() const { return bIsActive; }

	// Client -> Server request to flip current state
	UFUNCTION(Server, Reliable)
	void Server_RequestToggle();

	// Server-side authority setter (used internally)
	UFUNCTION(Server, Reliable)
	void Server_SetActive(bool bEnable);

	// ---- EMF level control ----
	// Server only. Called as scan updates
	UFUNCTION(Server, Reliable)
	void Server_SetEMFLevel(int32 NewLevel);

protected:
	// ---- Components ----

	// Visual body mesh. You attach BP static meshes under this.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMF")
	TObjectPtr<UStaticMeshComponent> EMFMesh;

	// Looping hum / crackle while active (owner-local)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EMF")
	TObjectPtr<UAudioComponent> HumAudioComp;

	// Power toggle one-shot sounds (owner-local)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMF|Audio")
	TObjectPtr<USoundBase> PowerOnSFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EMF|Audio")
	TObjectPtr<USoundBase> PowerOffSFX = nullptr;

	// ---- Replicated state ----

	// Power state of device
	UPROPERTY(ReplicatedUsing = OnRep_IsActive, BlueprintReadOnly, Category = "EMF")
	bool bIsActive = false;

	UFUNCTION()
	void OnRep_IsActive();

	// Spike level 0..5 (how many LEDs should glow)
	UPROPERTY(ReplicatedUsing = OnRep_EMFLevel, BlueprintReadOnly, Category = "EMF")
	int32 EMFLevel = 0;

	UFUNCTION()
	void OnRep_EMFLevel();

	// ---- LED / visual runtime control ----

	// Dynamic material instances for each LED bulb.
	// Index 0 = low/green ... index 4 = high/red
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LEDMIDs;

	// Build LEDMIDs array by finding child StaticMeshComponents tagged "EMF_LED"
	void CacheLEDMaterials();

	// Apply brightness to each LED based on EMFLevel and bIsActive
	void UpdateLEDVisuals();

	// Sync hum audio, scan timer, SFX when power changes
	void ApplyActiveState();

	// ---- Scan logic ----

	// Server timer while active
	FTimerHandle ScanTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "EMF|Scan")
	float ScanRadius = 800.f;


	// Server-side pulse. Pulls BinderCurrentEMFLevel from GameState and mirrors it
	void DoServerScanForEMF();

	// Which socket to attach to on the character mesh
	virtual FName GetAttachSocketName() const override
	{
		return TEXT("hand_R_EMF");
	}

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};


