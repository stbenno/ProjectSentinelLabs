// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter/Animation/SFW_TPAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Core/Components/SFW_EquipmentManagerComponent.h"

USFW_TPAnimInstance::USFW_TPAnimInstance() {}

void USFW_TPAnimInstance::NativeInitializeAnimation() {
	Super::NativeInitializeAnimation();
	OwningPawn = TryGetPawnOwner();
	if (OwningPawn) {
		EquipComp = OwningPawn->FindComponentByClass<USFW_EquipmentManagerComponent>();
	}
}

void USFW_TPAnimInstance::NativeUpdateAnimation(float DeltaSeconds) {
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningPawn) {
		OwningPawn = TryGetPawnOwner();
	}

	// movement data
	const FVector Vel = OwningPawn ? OwningPawn->GetVelocity() : FVector::ZeroVector;
	Speed = Vel.Size2D();

	const ACharacter* C = Cast<ACharacter>(OwningPawn);
	const UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
	bIsCrouched = Move ? Move->IsCrouching() : false;

	if (Speed > 3.f && OwningPawn) {
		const FRotator Yaw(0.f, OwningPawn->GetActorRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
		const FVector Rt = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(
			FVector::DotProduct(Vel, Rt),
			FVector::DotProduct(Vel, Fwd)));
	}
	else {
		Direction = 0.f;
	}

	// controller pitch for aim offset
	const AController* Ctrl = C ? C->GetController() : nullptr;
	if (Ctrl) {
		const float RawPitch = Ctrl->GetControlRotation().Pitch;
		AimPitch = FRotator::NormalizeAxis(RawPitch); // ~[-180,180], you will clamp in AnimBP if needed
	}
	else {
		AimPitch = 0.f;
	}

	// equipment state
	if (!EquipComp && OwningPawn) {
		EquipComp = OwningPawn->FindComponentByClass<USFW_EquipmentManagerComponent>();
	}

	HeldItemType = EquipComp ? EquipComp->GetHeldItemType() : EHeldItemType::None;
	EquipState = EquipComp ? EquipComp->GetEquipState() : EEquipState::Idle;

	bIsEquipping = (EquipState == EEquipState::Equipping);
	bIsHolding = (EquipState == EEquipState::Holding);
	bHasItemEquipped = (HeldItemType != EHeldItemType::None) &&
		(EquipState == EEquipState::Holding ||
			EquipState == EEquipState::Equipping ||
			EquipState == EEquipState::Using);
}
