#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFW_AnomalyController.generated.h"

class ARoomVolume;
class ASFW_GameState;

DECLARE_LOG_CATEGORY_EXTERN(LogAnomalyController, Log, All);

/** Drives site “anomaly” behaviors and room-targeted actions. */
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

	// ---- Decision loop ----
	void PickRooms();
	void BuildDefaultActionTable();

	UFUNCTION() void DecisionTick();
	UFUNCTION() void ServerTick();

	// ---- State ----
	UPROPERTY(VisibleAnywhere, Category = "Rooms")
	ARoomVolume* BaseRoom = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Rooms")
	ARoomVolume* RiftRoom = nullptr;

private:
	FTimerHandle DecisionHandle;
	FTimerHandle TickHandle;

	ASFW_GameState* GS() const;
};
