// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "SFW_WalkieTalkie.generated.h"

/**
 * Handheld radio. Owning this in inventory allows long-range comms.
 * Holding it (PrimaryUse) will later key radio transmit.
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_WalkieTalkie : public ASFW_EquippableBase
{
	GENERATED_BODY()

public:
	ASFW_WalkieTalkie();

	// This item grants access to radio comms
	virtual bool GrantsRadioComms_Implementation() const override { return true; }

	// Override socket if you want a specific grip pose for radio
	// You can change this socket name in editor/skeleton later
	virtual FName GetAttachSocketName() const override
	{
		return TEXT("hand_R_EMF"); // or "SCK_Hand_R_Radio" once you add that socket
	}

	// Called when player "uses" the walkie (press IA_Use while it's the active hand item)
	virtual void PrimaryUse() override;

protected:
	// later: audio component for active transmit beep, speaker sound, etc
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Walkie")
	// TObjectPtr<UAudioComponent> RadioAudio;
};

