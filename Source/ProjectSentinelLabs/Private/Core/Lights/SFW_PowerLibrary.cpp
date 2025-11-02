// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Lights/SFW_PowerLibrary.h"
#include "Core/Components/SFW_LampControllerComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogSFWPower);

UWorld* USFW_PowerLibrary::GetWorldChecked(UObject* WorldContextObject)
{
	check(WorldContextObject);
	UWorld* World = WorldContextObject->GetWorld();
	check(World);
	return World;
}

bool USFW_PowerLibrary::IsServer(UWorld* World)
{
	return World && (World->GetNetMode() != NM_Client);
}

void USFW_PowerLibrary::ForEachLamp(UWorld* World, TFunctionRef<void(USFW_LampControllerComponent*)> Fn)
{
	if (!World) return;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;

		if (USFW_LampControllerComponent* L = A->FindComponentByClass<USFW_LampControllerComponent>())
		{
			Fn(L);
		}
	}
}

void USFW_PowerLibrary::ForEachLampInRoom(UWorld* World, FName RoomId, TFunctionRef<void(USFW_LampControllerComponent*)> Fn)
{
	if (!World) return;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;

		if (USFW_LampControllerComponent* L = A->FindComponentByClass<USFW_LampControllerComponent>())
		{
			if (L->RoomId == RoomId)   // <- was GetRoomId()
			{
				Fn(L);
			}
		}
	}
}

void USFW_PowerLibrary::BlackoutSite(UObject* WorldContextObject, float Seconds)
{
	UWorld* W = GetWorldChecked(WorldContextObject);
	const int bAuth = IsServer(W) ? 1 : 0;

	int32 Total = 0;
	ForEachLamp(W, [&](USFW_LampControllerComponent* L)
		{
			++Total;
			if (bAuth) { L->SetState(ELampState::Off, Seconds); }
		});

	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutSite] Sec=%.2f Auth=%d"), Seconds, bAuth);
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutSite] TotalLamps=%d"), Total);
}


void USFW_PowerLibrary::BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds)
{
	UWorld* W = GetWorldChecked(WorldContextObject);
	const int bAuth = IsServer(W) ? 1 : 0;

	int32 Total = 0, Matched = 0;
	ForEachLamp(W, [&](USFW_LampControllerComponent* L)
		{
			++Total;
			if (L->RoomId == RoomId)     // <- was GetRoomId()
			{
				++Matched;
				if (bAuth) { L->SetState(ELampState::Off, Seconds); }
			}
		});

	UE_LOG(LogSFWPower, Log,
		TEXT("[BlackoutRoom] Room=%s Sec=%.2f Auth=%d"),
		*RoomId.ToString(), Seconds, bAuth);
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutRoom] TotalLamps=%d Matched=%d"), Total, Matched);
}

void USFW_PowerLibrary::FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds)
{
	UWorld* W = GetWorldChecked(WorldContextObject);
	const int bAuth = IsServer(W) ? 1 : 0;

	int32 Total = 0, Matched = 0;
	ForEachLamp(W, [&](USFW_LampControllerComponent* L)
		{
			++Total;
			if (L->RoomId == RoomId)     // <- was GetRoomId()
			{
				++Matched;
				if (bAuth) { L->SetState(ELampState::Flicker, Seconds); }
			}
		});

	UE_LOG(LogSFWPower, Log,
		TEXT("[FlickerRoom] Room=%s Sec=%.2f Auth=%d"),
		*RoomId.ToString(), Seconds, bAuth);
	UE_LOG(LogSFWPower, Log, TEXT("[FlickerRoom] TotalLamps=%d Matched=%d"), Total, Matched);
}