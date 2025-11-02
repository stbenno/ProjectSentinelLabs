// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Lights/SFW_PowerLibrary.h"
#include "Core/Components/SFW_LampControllerComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(LogSFWPower);

namespace {
	inline bool IsServer(const UWorld* W) { return W && W->GetNetMode() != NM_Client; }

	template<typename PredT>
	void ForEachLamp(UWorld* W, PredT Pred) {
		for (TActorIterator<AActor> It(W); It; ++It) {
			if (AActor* A = *It) {
				if (USFW_LampControllerComponent* L = A->FindComponentByClass<USFW_LampControllerComponent>()) {
					Pred(L);
				}
			}
		}
	}
}

void USFW_PowerLibrary::BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds)
{
	UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!W) { UE_LOG(LogSFWPower, Warning, TEXT("[BlackoutRoom] No World")); return; }
	const bool bAuth = IsServer(W);
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutRoom] Room=%s Sec=%.2f Auth=%d"), *RoomId.ToString(), Seconds, bAuth);
	if (!bAuth || RoomId.IsNone()) return;

	int32 Total = 0, Touched = 0;
	ForEachLamp(W, [RoomId, Seconds, &Total, &Touched](USFW_LampControllerComponent* L) {
		++Total;
		if (L && L->RoomId == RoomId) {
			++Touched;
			UE_LOG(LogSFWPower, Verbose, TEXT("[BlackoutRoom] -> %s Room=%s"),
				*L->GetOwner()->GetName(), *L->RoomId.ToString());
			L->SetState(ELampState::Off, Seconds);
		}
		});
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutRoom] TotalLamps=%d Matched=%d"), Total, Touched);
}

void USFW_PowerLibrary::BlackoutSite(UObject* WorldContextObject, float Seconds)
{
	UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!W) { UE_LOG(LogSFWPower, Warning, TEXT("[BlackoutSite] No World")); return; }
	const bool bAuth = IsServer(W);
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutSite] Sec=%.2f Auth=%d"), Seconds, bAuth);
	if (!bAuth) return;

	int32 Total = 0;
	ForEachLamp(W, [Seconds, &Total](USFW_LampControllerComponent* L) {
		++Total;
		if (L) {
			UE_LOG(LogSFWPower, Verbose, TEXT("[BlackoutSite] -> %s Room=%s"),
				*L->GetOwner()->GetName(), *L->RoomId.ToString());
			L->SetState(ELampState::Off, Seconds);
		}
		});
	UE_LOG(LogSFWPower, Log, TEXT("[BlackoutSite] TotalLamps=%d"), Total);
}

void USFW_PowerLibrary::FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds)
{
	UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!W) { UE_LOG(LogSFWPower, Warning, TEXT("[FlickerRoom] No World")); return; }
	const bool bAuth = IsServer(W);
	UE_LOG(LogSFWPower, Log, TEXT("[FlickerRoom] Room=%s Sec=%.2f Auth=%d"), *RoomId.ToString(), Seconds, bAuth);
	if (!bAuth || RoomId.IsNone()) return;

	int32 Total = 0, Touched = 0;
	ForEachLamp(W, [RoomId, Seconds, &Total, &Touched](USFW_LampControllerComponent* L) {
		++Total;
		if (L && L->RoomId == RoomId) {
			++Touched;
			UE_LOG(LogSFWPower, Verbose, TEXT("[FlickerRoom] -> %s Room=%s"),
				*L->GetOwner()->GetName(), *L->RoomId.ToString());
			L->SetState(ELampState::Flicker, Seconds);
		}
		});
	UE_LOG(LogSFWPower, Log, TEXT("[FlickerRoom] TotalLamps=%d Matched=%d"), Total, Touched);
}
