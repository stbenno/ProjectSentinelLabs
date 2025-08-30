// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Components/SFW_EquipmentManagerComponent.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Core/Actors/SFW_HeadLamp.h"

USFW_EquipmentManagerComponent::USFW_EquipmentManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Inventory.SetNumZeroed(kMaxInventorySlots);
}

void USFW_EquipmentManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerChar = Cast<ACharacter>(GetOwner());
	if (Inventory.Num() != kMaxInventorySlots)
	{
		Inventory.SetNumZeroed(kMaxInventorySlots);
	}
}

void USFW_EquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USFW_EquipmentManagerComponent, Inventory);
	DOREPLIFETIME(USFW_EquipmentManagerComponent, ActiveHandItem);
	DOREPLIFETIME(USFW_EquipmentManagerComponent, HeadLampRef);
}

void USFW_EquipmentManagerComponent::Server_ToggleHeadLamp_Implementation()
{
	if (ASFW_HeadLamp* Lamp = Cast<ASFW_HeadLamp>(HeadLampRef))
	{
		Lamp->ToggleLamp(); // flips & replicates
	}
}

bool USFW_EquipmentManagerComponent::HasFreeInventorySlot(int32& OutIndex) const
{
	for (int32 i = 0; i < kMaxInventorySlots; ++i)
	{
		if (Inventory[i] == nullptr)
		{
			OutIndex = i; return true;
		}
	}
	OutIndex = INDEX_NONE; return false;
}

bool USFW_EquipmentManagerComponent::IsInventoryFull() const
{
	int32 Dummy; return !HasFreeInventorySlot(Dummy);
}

ASFW_EquippableBase* USFW_EquipmentManagerComponent::GetItemInSlot(int32 Index) const
{
	return IsValidSlotIndex(Index) ? Inventory[Index] : nullptr;
}

bool USFW_EquipmentManagerComponent::SetItemInSlot_Internal(int32 Index, ASFW_EquippableBase* Item)
{
	if (!IsValidSlotIndex(Index)) return false;
	Inventory[Index] = Item;
	return true;
}

bool USFW_EquipmentManagerComponent::ClearSlot_Internal(int32 Index)
{
	if (!IsValidSlotIndex(Index)) return false;
	if (Inventory[Index] && Inventory[Index] == ActiveHandItem)
	{
		ActiveHandItem = nullptr;
	}
	Inventory[Index] = nullptr;
	return true;
}

void USFW_EquipmentManagerComponent::SetHeadLamp(ASFW_EquippableBase* InHeadLamp)
{
	HeadLampRef = InHeadLamp;
}

void USFW_EquipmentManagerComponent::OnRep_ActiveHandItem()
{
	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::ApplyActiveVisuals()
{
	// Hide all inventory items by default
	for (ASFW_EquippableBase* Item : Inventory)
	{
		if (Item) { Item->OnUnequipped(); }
	}

	// Show/attach the active one
	if (ActiveHandItem && OwnerChar)
	{
		ActiveHandItem->OnEquipped(OwnerChar);
	}
}

// ---- Server RPCs ----

void USFW_EquipmentManagerComponent::Server_AddItemToInventory_Implementation(ASFW_EquippableBase* Item, bool bAutoEquipIfHandEmpty)
{
	if (!Item) return;
	int32 FreeIndex;
	if (!HasFreeInventorySlot(FreeIndex)) return; // block pickup if full

	SetItemInSlot_Internal(FreeIndex, Item);

	// Auto-equip if the hand is empty
	if (bAutoEquipIfHandEmpty && ActiveHandItem == nullptr)
	{
		ActiveHandItem = Item;
	}
	ApplyActiveVisuals(); // update server visuals; clients get via OnRep
}

void USFW_EquipmentManagerComponent::Server_EquipSlot_Implementation(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex)) return;
	ASFW_EquippableBase* Candidate = Inventory[SlotIndex];
	if (!Candidate) return;

	ActiveHandItem = Candidate;
	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::Server_UnequipActive_Implementation()
{
	ActiveHandItem = nullptr;
	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::Server_DropActive_Implementation()
{
	// For now: just clear the reference; later spawn a pickup actor and place it at hand
	if (!ActiveHandItem) return;

	// Find which slot holds it
	for (int32 i = 0; i < kMaxInventorySlots; ++i)
	{
		if (Inventory[i] == ActiveHandItem)
		{
			ClearSlot_Internal(i);
			break;
		}
	}
	ActiveHandItem = nullptr;
	ApplyActiveVisuals();
}