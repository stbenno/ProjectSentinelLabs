// Fill out your copyright notice in the Description page of Project Settings.

// SFW_AnomalyPropComponent.cpp

#include "Core/Components/SFW_AnomalyPropComponent.h"

#include "Core/Lights/SFW_PowerLibrary.h"
#include "GameFramework/Actor.h"
#include "Core/Rooms/RoomVolume.h"
#include "Engine/World.h"

USFW_AnomalyPropComponent::USFW_AnomalyPropComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false); // only tick while pulsing
}

void USFW_AnomalyPropComponent::BeginPlay()
{
	Super::BeginPlay();
	// No caching here; we cache at pulse start so props that move still work.
}

void USFW_AnomalyPropComponent::TriggerAnomalyPulse(float PulseDurationSec)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[AnomalyProp] TriggerAnomalyPulse Owner=%s Duration=%.2f"),
		*GetNameSafe(Owner),
		PulseDurationSec);

	const float Now = World->GetTimeSeconds();

	// Cache current transform as the rest pose for this pulse
	BaseLocation = Owner->GetActorLocation();
	BaseRotation = Owner->GetActorRotation();

	PulseDuration = (PulseDurationSec > 0.f) ? PulseDurationSec : DefaultPulseDuration;
	if (PulseDuration <= 0.f)
	{
		return;
	}

	PulseStartTime = Now;
	bPulseActive = true;
	SetComponentTickEnabled(true);

	// Server marks this actor as an EMF source for the pulse window
	if (Owner->HasAuthority())
	{
		const float EMFSeconds = PulseDuration + FMath::Max(0.f, EMFSourceExtraSeconds);
		USFW_PowerLibrary::MakeActorEMFSource(this, Owner, EMFSeconds);
	}
}

void USFW_AnomalyPropComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPulseActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		StopPulse();
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float Elapsed = Now - PulseStartTime;

	if (Elapsed >= PulseDuration)
	{
		StopPulse();
		return;
	}

	// Basic bob + twist:
	// - vertical bob: sin(phase) * MaxOffset
	// - yaw wobble:   sin(phase * 2) * MaxAngleDeg
	const float Phase = Elapsed * PulseFrequency * 2.f * UE_PI;
	const float OffsetZ = FMath::Sin(Phase) * MaxOffset;
	const float ExtraYaw = FMath::Sin(Phase * 2.f) * MaxAngleDeg;
	
	FVector NewLocation = BaseLocation;
	NewLocation.Z += OffsetZ;

	FRotator NewRotation = BaseRotation;
	NewRotation.Yaw += ExtraYaw;

	Owner->SetActorLocationAndRotation(
		NewLocation,
		NewRotation,
		false,
		nullptr,
		ETeleportType::None
	);
}

void USFW_AnomalyPropComponent::StopPulse()
{
	bPulseActive = false;
	SetComponentTickEnabled(false);

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocationAndRotation(
			BaseLocation,
			BaseRotation,
			false,
			nullptr,
			ETeleportType::None
		);
	}
}


