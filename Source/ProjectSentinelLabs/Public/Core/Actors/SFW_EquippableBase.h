// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFW_EquippableBase.generated.h"

UCLASS()
class PROJECTSENTINELLABS_API ASFW_EquippableBase : public AActor
{
	GENERATED_BODY()

public:
	ASFW_EquippableBase();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equippable")
	TObjectPtr<USkeletalMeshComponent> Mesh;

public:
	// Called by the manager when this becomes the active hand item
	virtual void OnEquipped(ACharacter* NewOwnerChar);

	// Called by the manager when it is put away / unequipped
	virtual void OnUnequipped();

	// Input hooks (no behavior yet)
	virtual void PrimaryUse() {}
	virtual void SecondaryUse() {}

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

protected:
	// Basic attach/detach helpers
	void AttachToCharacter(ACharacter* Char, FName Socket);
	void DetachFromCharacter();
};