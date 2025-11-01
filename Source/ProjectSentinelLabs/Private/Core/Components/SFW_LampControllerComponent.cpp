// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Components/SFW_LampControllerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"

USFW_LampControllerComponent::USFW_LampControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFW_LampControllerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// Server applies initial state which will replicate.
	}
	CreateMIDsIfNeeded();
	ApplyState();
}

void USFW_LampControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USFW_LampControllerComponent, State);
}

void USFW_LampControllerComponent::OnRep_State()
{
	ApplyState();
}

void USFW_LampControllerComponent::SetState(ELampState NewState, float OptionalDurationSeconds)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// Server-only authority to change replicated state.
		return;
	}

	// Handle optional blackout timer
	if (OptionalDurationSeconds > 0.f)
	{
		if (NewState == ELampState::Off)
		{
			PrevStateBeforeBlackout = State == ELampState::Off ? PrevStateBeforeBlackout : State;
			State = ELampState::Off;
			OnRep_State();
			StartBlackoutRestoreTimer(OptionalDurationSeconds);
			return;
		}
		// Non-off timed states just set and schedule a restore to Off
		State = NewState;
		OnRep_State();
		StartBlackoutRestoreTimer(OptionalDurationSeconds);
		return;
	}

	State = NewState;
	OnRep_State();
}

void USFW_LampControllerComponent::StartBlackoutRestoreTimer(float DurationSeconds)
{
	// Clear and set restore timer back to previous state (or On if not set)
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(RestoreTimer);
		W->GetTimerManager().SetTimer(
			RestoreTimer,
			[this]()
			{
				if (!GetOwner() || !GetOwner()->HasAuthority()) return;
				const ELampState Restore = (PrevStateBeforeBlackout == ELampState::Off) ? ELampState::On : PrevStateBeforeBlackout;
				State = Restore;
				OnRep_State();
			},
			DurationSeconds,
			false
		);
	}
}

void USFW_LampControllerComponent::ApplyState()
{
	StopFlicker();

	switch (State)
	{
	case ELampState::On:
		ApplyEmissive(OnEmissive);
		break;
	case ELampState::Off:
		ApplyEmissive(OffEmissive);
		break;
	case ELampState::Flicker:
		StartFlicker();
		break;
	default:
		break;
	}
}

void USFW_LampControllerComponent::StartFlicker()
{
	// Kick an immediate tick then schedule the next
	TickFlickerOnce();
}

void USFW_LampControllerComponent::StopFlicker()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(FlickerTimer);
	}
}

void USFW_LampControllerComponent::TickFlickerOnce()
{
	// Random emissive between off and on, then reschedule
	const float Emissive = FMath::FRandRange(OffEmissive, OnEmissive);
	ApplyEmissive(Emissive);

	const float Interval = FMath::FRandRange(FlickerIntervalMin, FlickerIntervalMax);
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			FlickerTimer,
			this,
			&USFW_LampControllerComponent::TickFlickerOnce,
			Interval,
			false
		);
	}
}

void USFW_LampControllerComponent::ApplyEmissive(float Scalar)
{
	CreateMIDsIfNeeded();
	for (UMaterialInstanceDynamic* MID : MIDs)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(EmissiveParamName, Scalar);
		}
	}
}

void USFW_LampControllerComponent::RebuildMaterialInstances()
{
	ClearMIDs();
	CreateMIDsIfNeeded();
	ApplyState();
}

void USFW_LampControllerComponent::CreateMIDsIfNeeded()
{
	if (MIDs.Num() > 0) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* MC : Meshes)
	{
		if (!MC) continue;
		const int32 MatCount = MC->GetNumMaterials();
		for (int32 i = 0; i < MatCount; ++i)
		{
			if (UMaterialInterface* MI = MC->GetMaterial(i))
			{
				if (UMaterialInstanceDynamic* MID = MC->CreateAndSetMaterialInstanceDynamic(i))
				{
					MIDs.Add(MID);
				}
			}
		}
	}
}

void USFW_LampControllerComponent::ClearMIDs()
{
	MIDs.Reset();
}



