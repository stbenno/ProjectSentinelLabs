// SFW_AnomalyController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFW_AnomalyController.generated.h"

class ARoomVolume;
class ASFW_GameState;

DECLARE_LOG_CATEGORY_EXTERN(LogAnomalyController, Log, All);

/**
 * Picks Base / Rift rooms and marks round state.
 * All decision scheduling is handled by ASFW_AnomalyDecisionSystem.
 */
UCLASS()
class PROJECTSENTINELLABS_API ASFW_AnomalyController : public AActor
{
	GENERATED_BODY()

public:
	ASFW_AnomalyController();

	/** Entry point from game mode or level blueprint. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Anomaly")
	void StartRound();

protected:
	virtual void BeginPlay() override;

	// ---- Room selection ----
	void PickRooms();

	/** Base room for the round (non-safe, non-hallway). */
	UPROPERTY(VisibleAnywhere, Category = "Rooms")
	ARoomVolume* BaseRoom = nullptr;

	/** Rift / anomaly focus room for the round (non-safe, non-hallway). */
	UPROPERTY(VisibleAnywhere, Category = "Rooms")
	ARoomVolume* RiftRoom = nullptr;

private:
	ASFW_GameState* GS() const;
	bool IsForbiddenRoom(ARoomVolume* R) const;
};
