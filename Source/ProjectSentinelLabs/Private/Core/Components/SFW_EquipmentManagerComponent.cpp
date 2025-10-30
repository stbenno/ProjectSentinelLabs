// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Components/SFW_EquipmentManagerComponent.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "Core/Actors/SFW_HeadLamp.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Core/Actors/SFW_EMFDevice.h"


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
	DOREPLIFETIME(USFW_EquipmentManagerComponent, HeldItem);
	DOREPLIFETIME(USFW_EquipmentManagerComponent, Equip);
}

// ---------- Queries ----------

bool USFW_EquipmentManagerComponent::HasFreeInventorySlot(int32& OutIndex) const
{
	for (int32 i = 0; i < kMaxInventorySlots; ++i)
	{
		if (Inventory[i] == nullptr)
		{
			OutIndex = i;
			return true;
		}
	}
	OutIndex = INDEX_NONE;
	return false;
}

bool USFW_EquipmentManagerComponent::IsInventoryFull() const
{
	int32 Dummy;
	return !HasFreeInventorySlot(Dummy);
}

ASFW_EquippableBase* USFW_EquipmentManagerComponent::GetItemInSlot(int32 Index) const
{
	return IsValidSlotIndex(Index) ? Inventory[Index] : nullptr;
}

bool USFW_EquipmentManagerComponent::CanUseRadioComms() const
{
	// Rule: if you have *any* item that returns GrantsRadioComms() == true,
	// you are allowed to transmit on radio.
	for (ASFW_EquippableBase* Item : Inventory)
	{
		if (Item && Item->GrantsRadioComms())
		{
			return true;
		}
	}
	return false;
}

// ---------- Helpers ----------

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
	UE_LOG(LogTemp, Log, TEXT("[EquipMgr] ApplyActiveVisuals Owner=%s ActiveHandItem=%s"),
		*GetOwner()->GetName(),
		ActiveHandItem ? *ActiveHandItem->GetName() : TEXT("NULL"));

	// Hide everything first
	for (ASFW_EquippableBase* Item : Inventory)
	{
		if (Item)
		{
			Item->OnUnequipped();
		}
	}

	// Then show/attach the active item
	if (ActiveHandItem && OwnerChar)
	{
		UE_LOG(LogTemp, Log, TEXT("[EquipMgr] Equipping %s to %s"),
			*ActiveHandItem->GetName(),
			*OwnerChar->GetName());

		ActiveHandItem->OnEquipped(OwnerChar);
	}
}



ASFW_EMFDevice* USFW_EquipmentManagerComponent::FindEMF() const
{
	for (ASFW_EquippableBase* Item : Inventory)
	{
		if (!Item) continue;

		if (ASFW_EMFDevice* EMF = Cast<ASFW_EMFDevice>(Item))
		{
			return EMF;
		}
	}
	return nullptr;
}


// ---------- Server RPCs ----------

void USFW_EquipmentManagerComponent::Server_AddItemToInventory_Implementation(ASFW_EquippableBase* Item, bool bAutoEquipIfHandEmpty)
{
	UE_LOG(LogTemp, Log, TEXT("[EquipMgr] AddItemToInventory called on %s. Item=%s AutoEquip=%d"),
		*GetOwner()->GetName(),
		Item ? *Item->GetName() : TEXT("NULL"),
		bAutoEquipIfHandEmpty ? 1 : 0);

	if (!Item) return;

	int32 FreeIndex;
	if (!HasFreeInventorySlot(FreeIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EquipMgr] Inventory full for %s"), *GetOwner()->GetName());
		return;
	}

	SetItemInSlot_Internal(FreeIndex, Item);
	UE_LOG(LogTemp, Log, TEXT("[EquipMgr] Stored %s in slot %d"), *Item->GetName(), FreeIndex);

	if (bAutoEquipIfHandEmpty && ActiveHandItem == nullptr)
	{
		ActiveHandItem = Item;
		HeldItem = EHeldItemType::Flashlight; // temp
		Equip = EEquipState::Holding;

		UE_LOG(LogTemp, Log, TEXT("[EquipMgr] Auto-equipped %s as ActiveHandItem"), *Item->GetName());
	}

	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::Server_EquipSlot_Implementation(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex)) return;

	ASFW_EquippableBase* Candidate = Inventory[SlotIndex];
	if (!Candidate) return;

	ActiveHandItem = Candidate;

	// TEMP mapping until items report their type
	HeldItem = EHeldItemType::Flashlight;
	Equip = EEquipState::Holding;

	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::Server_UnequipActive_Implementation()
{
	ActiveHandItem = nullptr;
	HeldItem = EHeldItemType::None;
	Equip = EEquipState::Idle;

	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::Server_DropActive_Implementation()
{
	if (!ActiveHandItem) return;

	// Remove from inventory
	for (int32 i = 0; i < kMaxInventorySlots; ++i)
	{
		if (Inventory[i] == ActiveHandItem)
		{
			ClearSlot_Internal(i);
			break;
		}
	}

	ActiveHandItem = nullptr;
	HeldItem = EHeldItemType::None;
	Equip = EEquipState::Idle;

	// (Future: spawn pickup actor with physics here)

	ApplyActiveVisuals();
}

void USFW_EquipmentManagerComponent::UseActiveLocal()
{
	// Nothing equipped yet. Do nothing.
	if (!ActiveHandItem)
	{
		return;
	}

	// We have an item in hand. Now it's a real use.
	UE_LOG(LogTemp, Log, TEXT("UseActiveLocal: Using %s"), *ActiveHandItem->GetName());

	ActiveHandItem->PrimaryUse(); // item handles its own RPC if needed
}

void USFW_EquipmentManagerComponent::Server_ToggleHeadLamp_Implementation()
{
	if (ASFW_HeadLamp* Lamp = Cast<ASFW_HeadLamp>(HeadLampRef))
	{
		Lamp->ToggleLamp(); // headlamp replicates state internally
	}
}
