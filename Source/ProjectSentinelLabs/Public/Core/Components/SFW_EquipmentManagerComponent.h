// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFW_EquipmentManagerComponent.generated.h"

class ASFW_EquippableBase;
class ACharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTSENTINELLABS_API USFW_EquipmentManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFW_EquipmentManagerComponent();
	static constexpr int32 kMaxInventorySlots = 3;

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Replicated State ---
	UPROPERTY(Replicated)
	TArray<TObjectPtr<ASFW_EquippableBase>> Inventory;

	// SFW_EquipmentManagerComponent.h
	UFUNCTION(Server, Reliable)
	void Server_ToggleHeadLamp();

	/** Active hand item (also exists in Inventory). */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveHandItem)
	TObjectPtr<ASFW_EquippableBase> ActiveHandItem = nullptr;

	/** Dedicated headlamp reference (not in Inventory). */
	UPROPERTY(Replicated)
	TObjectPtr<ASFW_EquippableBase> HeadLampRef = nullptr;

	// --- Queries ---
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool HasFreeInventorySlot(int32& OutIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsInventoryFull() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsHandEmpty() const { return ActiveHandItem == nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ASFW_EquippableBase* GetItemInSlot(int32 Index) const;

	// --- Core actions (server auth) ---
	UFUNCTION(Server, Reliable)
	void Server_AddItemToInventory(ASFW_EquippableBase* Item, bool bAutoEquipIfHandEmpty);

	UFUNCTION(Server, Reliable)
	void Server_EquipSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void Server_UnequipActive();

	UFUNCTION(Server, Reliable)
	void Server_DropActive(); // spawns a pickup later; for now just clears

	// Headlamp setter
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetHeadLamp(ASFW_EquippableBase* InHeadLamp);

protected:
	// RepNotify
	UFUNCTION()
	void OnRep_ActiveHandItem();

private:
	bool IsValidSlotIndex(int32 Index) const { return Index >= 0 && Index < kMaxInventorySlots; }
	bool SetItemInSlot_Internal(int32 Index, ASFW_EquippableBase* Item);
	bool ClearSlot_Internal(int32 Index);

	// Visual updates when ActiveHandItem changes (both server & client via OnRep)
	void ApplyActiveVisuals();

	// Cached owner character
	UPROPERTY()
	TObjectPtr<ACharacter> OwnerChar = nullptr;
};