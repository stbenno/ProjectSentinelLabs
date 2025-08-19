// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "SFW_PlayerBase.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UInputMappingContext;

UCLASS()
class PROJECTSENTINELLABS_API ASFW_PlayerBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASFW_PlayerBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	/* Local FP camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* FirstPersonCamera;

	/* Local-only First-Person Arms (OwnerOnly)*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* Mesh1P;

	/* Sprint State (replicated, useful for remote anim BP if needed) */
	UPROPERTY(ReplicatedUsing=OnRep_WantsToSprint, BlueprintReadOnly, Category = "Movement")
	bool bWantsToSprint = false;

	/* Speeds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchSpeed = 200.f;

	//Helper to set speed from current state
	void ApplyMovementSpeed();

	// RepNotify for bWantsToSprint
	UFUNCTION()
	void OnRep_WantsToSprint();

	// Server RPC to set sprint
	UFUNCTION(Server, Reliable)
	void ServerSetSprint(bool bSprint);

	// Enhanced Input assets (assign in the BP child)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultIMC = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Look = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Sprint = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Crouch = nullptr;

	// Handlers using Enhanced Input values
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SprintStarted(const FInputActionValue& Value);
	void SprintCompleted(const FInputActionValue& Value);
	void CrouchStarted(const FInputActionValue& Value) { Crouch(false); ApplyMovementSpeed(); }
	void CrouchCompleted(const FInputActionValue& Value) { UnCrouch(false); ApplyMovementSpeed(); }

	

	//Interact
	//void Interact(); //bound on client

	//UFUNCTION(Server, Reliable)
	//void Server_Interact(const FHitResult& Hit); //Executed on server


	
	
	void UpdateMeshVisibility();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
