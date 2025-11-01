// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFW_LampControllerComponent.generated.h"

UENUM(BlueprintType)
enum class ELampState : uint8
{
	On       UMETA(DisplayName = "On"),
	Flicker  UMETA(DisplayName = "Flicker"),
	Off      UMETA(DisplayName = "Off")
};

/** Per-lamp controller. Replicates state and drives emissive. */
UCLASS(ClassGroup = (SFW), meta = (BlueprintSpawnableComponent))
class PROJECTSENTINELLABS_API USFW_LampControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFW_LampControllerComponent();

	/** Current state (server sets; clients receive). */
	UPROPERTY(ReplicatedUsing = OnRep_State, BlueprintReadOnly, Category = "Lamp")
	ELampState State = ELampState::On;

	/** Set state (server only). Optional blackout duration. */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void SetState(ELampState NewState, float OptionalDurationSeconds = -1.f);

	/** Force re-scan of mesh components and rebuild MIDs. */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void RebuildMaterialInstances();

	/** Emissive scalar parameter name on the lamp material(s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Material")
	FName EmissiveParamName = TEXT("Glow");

	/** Emissive scalar values for On/Off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Material")
	float OnEmissive = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Material")
	float OffEmissive = 0.0f;

	/** Flicker timing range in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Flicker")
	float FlickerIntervalMin = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp|Flicker")
	float FlickerIntervalMax = 0.20f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_State();

private:
	/** Previous non-Off state for blackout restore. */
	UPROPERTY()
	ELampState PrevStateBeforeBlackout = ELampState::On;

	/** Dynamic material instances on all mesh materials. */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MIDs;

	FTimerHandle FlickerTimer;
	FTimerHandle RestoreTimer;

	void ApplyState();
	void StartFlicker();
	void StopFlicker();

	void TickFlickerOnce();
	void ApplyEmissive(float Scalar);

	void CreateMIDsIfNeeded();
	void ClearMIDs();

	/** Server-only helpers */
	void StartBlackoutRestoreTimer(float DurationSeconds);

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};