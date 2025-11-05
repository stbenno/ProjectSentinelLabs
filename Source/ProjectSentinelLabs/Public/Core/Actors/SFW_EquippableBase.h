// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Actors/Interface/SFW_InteractableInterface.h"
#include "SFW_EquippableBase.generated.h"

class AController;
class ACharacter;
class USkeletalMeshComponent;
class UPrimitiveComponent;

UCLASS()
class PROJECTSENTINELLABS_API ASFW_EquippableBase
	: public AActor
	, public ISFW_InteractableInterface
{
	GENERATED_BODY()

public:
	ASFW_EquippableBase();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equippable")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	// Cached default relative transform of the physics component (equipped pose)
	UPROPERTY()
	FTransform InitialPhysicsRelativeTransform;

	bool bHasCachedPhysicsRelativeTransform = false;

public:
	virtual void OnEquipped(ACharacter* NewOwnerChar);
	virtual void OnUnequipped();

	/** Called when the item is dropped into the world (with a small toss). */
	virtual void OnDropped(const FVector& DropLocation, const FVector& TossVelocity);

	// Primary / secondary use hooks (flashlight toggle, etc)
	virtual void PrimaryUse() {}
	virtual void SecondaryUse() {}

	USkeletalMeshComponent* GetMesh() const { return Mesh; }

	// Interactable interface
	virtual FText GetPromptText_Implementation() const override;
	virtual void Interact_Implementation(AController* InstigatorController) override;

protected:
	void AttachToCharacter(ACharacter* Char, FName Socket);
	void DetachFromCharacter();

	// Which component should get physics when dropped.
	virtual UPrimitiveComponent* GetPhysicsComponent() const;

	// Default attach point for handhelds; subclasses can override.
	virtual FName GetAttachSocketName() const { return TEXT("hand_R_Tool"); }

public:
	// Does having this item in inventory grant access to long-range radio comms?
	// Default false. Walkie subclass will override to true.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Voice")
	bool GrantsRadioComms() const;
	virtual bool GrantsRadioComms_Implementation() const { return false; }
};
