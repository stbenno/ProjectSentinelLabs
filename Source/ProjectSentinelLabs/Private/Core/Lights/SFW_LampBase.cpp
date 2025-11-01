// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Lights/SFW_LampBase.h"
#include "Components/StaticMeshComponent.h"

ASFW_LampBase::ASFW_LampBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Lamp = CreateDefaultSubobject<USFW_LampControllerComponent>(TEXT("Lamp"));
}

void ASFW_LampBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Lamp)
	{
		Lamp->RoomId = RoomId;
	}
}

void ASFW_LampBase::SetLampState(ELampState NewState, float OptionalDurationSeconds)
{
	if (!HasAuthority() || !Lamp) return;
	Lamp->SetState(NewState, OptionalDurationSeconds);
}