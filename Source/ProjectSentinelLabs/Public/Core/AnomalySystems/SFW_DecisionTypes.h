#pragma once

#include "Engine/DataTable.h"
#include "SFW_DecisionTypes.generated.h"

UENUM(BlueprintType)
enum class ESFWDecision : uint8 { Idle, LampFlicker, LockDoor, JamDoor, OpenDoor, CloseDoor, KnockDoor, JumpScare, Teleport, Trap, Patrol, Hunt, EvidenceT1, EvidenceT2, Characteristic };

USTRUCT(BlueprintType)
struct FSFWDecisionRow : public FTableRowBase {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) int32        Tier = 1;
	UPROPERTY(EditAnywhere) ESFWDecision Type = ESFWDecision::Idle;
	UPROPERTY(EditAnywhere) float        Weight = 1.f;
	UPROPERTY(EditAnywhere) float        CooldownSec = 6.f;
	UPROPERTY(EditAnywhere) float        Magnitude = 1.f;
	UPROPERTY(EditAnywhere) float        Duration = 2.f;
};

USTRUCT(BlueprintType)
struct FSFWDecisionPayload {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ESFWDecision Type = ESFWDecision::Idle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName        RoomId = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float        Magnitude = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float        Duration = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) AActor* Instigator = nullptr; // replace TWeakObjectPtr
};