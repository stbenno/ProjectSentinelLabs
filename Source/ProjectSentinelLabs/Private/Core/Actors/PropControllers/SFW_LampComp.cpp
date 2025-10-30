// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Actors/PropControllers/SFW_LampComp.h"

#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Core/AnomalySystems/SFW_AnomalyDecisionSystem.h"

USFW_LampComp::USFW_LampComp() { PrimaryComponentTick.bCanEverTick = false; SetIsReplicatedByDefault(true); }

void USFW_LampComp::BeginPlay()
{
	Super::BeginPlay();

	// Bind to decision system on all machines
	for (TActorIterator<ASFW_AnomalyDecisionSystem> It(GetWorld()); It; ++It)
	{
		if (auto* Sys = *It) { Sys->OnDecisionBP.AddDynamic(this, &USFW_LampComp::HandleDecisionBP); break; }
	}

	if (GetOwnerRole() == ROLE_Authority) RecomputeMode();
	else ApplyMode(true);
}

void USFW_LampComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USFW_LampComp, bHasPower);
	DOREPLIFETIME(USFW_LampComp, bDesiredOn);
	DOREPLIFETIME(USFW_LampComp, FlickerEndSec);
	DOREPLIFETIME(USFW_LampComp, Mode);
}

void USFW_LampComp::OnRep_Mode() { ApplyMode(true); }

void USFW_LampComp::ServerPlayerToggle_Implementation()
{
	bDesiredOn = !bDesiredOn;
	RecomputeMode();
}

void USFW_LampComp::OnPowerChanged(bool bPowered)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	bHasPower = bPowered;
	RecomputeMode();
}

void USFW_LampComp::HandleDecisionBP(const FSFWDecisionPayload& P)
{
	if (P.Type == ESFWDecision::LampFlicker && P.RoomId == OwningRoomId)
	{
		if (GetOwnerRole() == ROLE_Authority)
		{
			FlickerEndSec = GetWorld()->GetTimeSeconds() + P.Duration;
			RecomputeMode();
		}
		else if (Mode == ESFWLampMode::On)
		{
			BP_ApplyLampMode(ESFWLampMode::Flicker, P.Duration);
		}
	}
}

void USFW_LampComp::RecomputeMode()
{
	check(GetOwnerRole() == ROLE_Authority);
	const float Now = GetWorld()->GetTimeSeconds();
	ESFWLampMode NewMode = ESFWLampMode::Off;

	if (!bHasPower)                 NewMode = ESFWLampMode::NoPower;
	else if (Now < FlickerEndSec)   NewMode = ESFWLampMode::Flicker;
	else if (bDesiredOn)            NewMode = ESFWLampMode::On;

	if (NewMode != Mode) { Mode = NewMode; ApplyMode(false); }
}

void USFW_LampComp::ApplyMode(bool /*FromRep*/)
{
	float Remaining = 0.f;
	if (Mode == ESFWLampMode::Flicker)
		Remaining = FMath::Max(0.f, FlickerEndSec - GetWorld()->GetTimeSeconds());
	BP_ApplyLampMode(Mode, Remaining);
}