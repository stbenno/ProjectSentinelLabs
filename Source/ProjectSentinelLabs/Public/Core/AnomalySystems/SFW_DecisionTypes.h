#pragma once

#include "Engine/DataTable.h"
#include "SFW_DecisionTypes.generated.h"

UENUM(BlueprintType)
enum class ESFWDecision : uint8 {
    Idle, LampFlicker, LockDoor, JamDoor, OpenDoor, CloseDoor, KnockDoor,
    JumpScare, Teleport, Trap, Patrol, Hunt, EvidenceT1, EvidenceT2, Characteristic,
    BlackoutRoom   
};

USTRUCT(BlueprintType)
struct FSFWDecisionRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32        Tier = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) ESFWDecision Type = ESFWDecision::Idle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float        Weight = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float        CooldownSec = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float        Magnitude = 1.f; // use for intensity if needed
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float        Duration = 2.f;  // seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName        TargetRoomId = NAME_None; // NEW (optional; leave None to use fallback)
};

USTRUCT(BlueprintType)
struct FSFWDecisionPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) ESFWDecision Type = ESFWDecision::Idle;
    UPROPERTY(BlueprintReadWrite) FName        RoomId = NAME_None;
    UPROPERTY(BlueprintReadWrite) float        Magnitude = 1.f;
    UPROPERTY(BlueprintReadWrite) float        Duration = 2.f;

    // Weak on purpose; payloads are transient and should not pin actors for GC
    UPROPERTY() TWeakObjectPtr<AActor> Instigator = nullptr; // replaces raw AActor*
};