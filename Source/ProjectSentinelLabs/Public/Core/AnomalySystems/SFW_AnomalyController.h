// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/AnomalySystems/SFW_DecisionTypes.h"
#include "SFW_AnomalyController.generated.h"

class ASFW_GameState;
class ASFW_AnomalyDecisionSystem;
class ASFW_DoorBase;
class ARoomVolume;
class APawn;

/** Must match EAnomalyClass in GameState so we can forward-declare it here */
enum class EAnomalyClass : uint8;

/** High level actions the anomaly AI can request once per second */
enum class EAnomalyActionId : uint8
{
	DoNothing = 0,
	OpenEvidenceWindow,
	SpawnBaselineShade
};

/** Row used by weighted action selector */
struct FAnomalyActionRow
{
	EAnomalyActionId ActionId = EAnomalyActionId::DoNothing;

	float SanityMin = 0.f;
	float SanityMax = 100.f;

	bool  bRequiresInRift = true;

	int32 MinShades = 0;
	int32 MaxShades = 99;

	float GlobalCooldownSec = 5.f;
	float Weight = 1.f;
};

/**
 * Server-side anomaly director.
 * - Picks base room and rift room
 * - Spawns baseline shade
 * - Opens/closes evidence windows
 * - Relays per-room decisions (doors, lamps) from ASFW_AnomalyDecisionSystem
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_AnomalyController : public AActor
{
	GENERATED_BODY()
public:
	ASFW_AnomalyController();

	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void StartRound();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// Decision-system routing (lamp flicker, door slam etc)
	UFUNCTION()
	void HandleDecision(const FSFWDecisionPayload& P);

	// Lamp flicker path to BP. BP can find lamps in the room and play VFX.
	UFUNCTION(BlueprintImplementableEvent, Category = "Anomaly")
	void BP_HandleLampFlicker(const FSFWDecisionPayload& Payload);

	UPROPERTY(EditDefaultsOnly, Category = "Anomaly|Find")
	TSubclassOf<ASFW_DoorBase> DoorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Anomaly|Find")
	TSubclassOf<AActor> LampClass;

private:
	// ------------------------
	// Timers
	// ------------------------
	FTimerHandle DecisionHandle; // main 1 Hz director tick
	FTimerHandle TickHandle;     // misc housekeeping

	// ------------------------
	// Tunables
	// ------------------------
	// how far apart base and rift should prefer to be (bigger = prefers closer)
	UPROPERTY(EditAnywhere, Category = "Anomaly|Rooms")
	float RiftSigmaUU = 1200.f;

	// max live shades we allow
	UPROPERTY(EditAnywhere, Category = "Anomaly|Spawns")
	int32 MaxShadesAlive = 2;

	// sanity gate for spawning a baseline shade
	UPROPERTY(EditAnywhere, Category = "Anomaly|Spawns")
	float SpawnGateSanity = 85.f;

	// leash data for hunts (not yet used)
	UPROPERTY(EditAnywhere, Category = "Anomaly|Hunt")
	float HuntStartRadiusUU = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Anomaly|Hunt")
	float RiftLeashUU = 3500.f;

	// evidence trigger rules (server only. clients read replicated GameState)
	UPROPERTY(EditAnywhere, Category = "Anomaly|Evidence")
	float EvidenceSanityMin = 70.f;

	UPROPERTY(EditAnywhere, Category = "Anomaly|Evidence")
	float EvidenceSanityMax = 85.f;

	UPROPERTY(EditAnywhere, Category = "Anomaly|Evidence")
	float EvidenceWindowDurationSec = 5.f;

	UPROPERTY(EditAnywhere, Category = "Anomaly|Evidence")
	float EvidenceWindowCooldownSec = 75.f;

	UPROPERTY(EditAnywhere, Category = "Anomaly|Evidence")
	float EvidenceWindowFailsafeSec = 90.f;

	// which pawn class to spawn for baseline shade
	UPROPERTY(EditDefaultsOnly, Category = "Anomaly|Spawns")
	TSubclassOf<APawn> ShadePawnClass;

	// ------------------------
	// Internal state
	// ------------------------
	UPROPERTY() AActor* BaseRoom = nullptr;
	UPROPERTY() AActor* RiftRoom = nullptr;

	// last time we successfully opened an evidence window (server time seconds)
	float LastEvidenceFireTime = -1000.f;

	// ------------------------
	// Main loops
	// ------------------------
	void DecisionTick();  // once per second AI brain
	void ServerTick();    // placeholder for misc upkeep

	// ------------------------
	// Room helpers
	// ------------------------
	void PickRooms();
	float PathDistanceUU(const AActor* A, const AActor* B) const;
	float LowestSanityInRift() const;
	bool AnyPlayerInRift() const;

	// ------------------------
	// Shade helpers
	// ------------------------
	int32 CountActiveShades() const;
	void EnsureBaselineShade();
	void TryPunishShade(); // placeholder

	// ------------------------
	// Evidence helpers
	// ------------------------
	int32 PickEvidenceTypeForClass(EAnomalyClass Class) const;

	// ------------------------
	// GameState
	// ------------------------
	ASFW_GameState* GS() const;

	// ------------------------
	// Action selection
	// ------------------------
	void BuildDefaultActionTable();
	bool ActionOnCooldown(EAnomalyActionId Id, float Now) const;
	void MarkActionFired(EAnomalyActionId Id, float Now);
	const FAnomalyActionRow* ChooseAction(float Now, float Lowest, bool bInRift, int32 ShadesAlive) const;
	void ExecuteAction(const FAnomalyActionRow& Row, float Now, float Lowest, bool bInRift, int32 ShadesAlive);

	TArray<FAnomalyActionRow> ActionTable;
	TMap<uint8, float> LastFiredByAction; // key=(uint8)EAnomalyActionId -> last fire time

	// ------------------------
	// Safe-room cache
	// ------------------------
	static const FName TAG_BaseRoom;
	static const FName TAG_RiftRoom;
	static const FName TAG_ShadeSpawn;
	static const FName TAG_ShadeActor;

	void CacheSafeRooms();
	bool IsSafeRoom(FName RoomId) const { return SafeRoomIds.Contains(RoomId); }
	TSet<FName> SafeRoomIds;
};
