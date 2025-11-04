// Fill out your copyright notice in the Description page of Project Settings.

// SFW_AnomalyPropComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFW_AnomalyPropComponent.generated.h"

/**
 * Component for props the anomaly can "manipulate":
 * - Temporarily shakes / bobs the actor (visual pulse)
 * - Marks the actor as an EMF source for the same time window
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTSENTINELLABS_API USFW_AnomalyPropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFW_AnomalyPropComponent();

	/** Trigger a pulse. If DurationOverride <= 0, uses DefaultPulseDuration. */
	UFUNCTION(BlueprintCallable, Category = "Anomaly|Prop")
	void TriggerAnomalyPulse(float PulseDurationSec);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/** Max vertical offset (in cm) while pulsing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|Prop")
	float MaxOffset = 5.f;

	/** Max extra yaw angle (in degrees) while pulsing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|Prop")
	float MaxAngleDeg = 8.f;

	/** Oscillation frequency in cycles per second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|Prop")
	float PulseFrequency = 2.f;

	/** Default pulse duration if none is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|Prop")
	float DefaultPulseDuration = 2.f;

	/**
	 * Extra seconds to keep EMF_Source active beyond visual pulse.
	 * Set to 0 to match exactly, or >0 to let EMF linger briefly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anomaly|Prop")
	float EMFSourceExtraSeconds = 0.25f;

private:
	bool bPulseActive = false;
	float PulseStartTime = 0.f;
	float PulseDuration = 0.f;

	FVector BaseLocation = FVector::ZeroVector;
	FRotator BaseRotation = FRotator::ZeroRotator;

	void StopPulse();
};
