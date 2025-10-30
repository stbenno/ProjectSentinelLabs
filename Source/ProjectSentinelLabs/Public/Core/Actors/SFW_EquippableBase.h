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
	virtual void OnEquipped(ACharacter* NewOwnerChar);
	virtual void OnUnequipped();

	// Primary / secondary use hooks (flashlight toggle, etc)
	virtual void PrimaryUse() {}
	virtual void SecondaryUse() {}

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

protected:
	void AttachToCharacter(ACharacter* Char, FName Socket);
	void DetachFromCharacter();

	// Default attach point for handhelds; subclasses can override.
	virtual FName GetAttachSocketName() const { return TEXT("hand_R_Tool"); }

public:
	// Does having this item in inventory grant access to long-range radio comms?
	// Default false. Walkie subclass will override to true.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voice")
	bool GrantsRadioComms() const;
	virtual bool GrantsRadioComms_Implementation() const { return false; }
};
