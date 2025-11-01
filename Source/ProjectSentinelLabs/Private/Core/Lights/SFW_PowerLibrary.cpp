// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Lights/SFW_PowerLibrary.h"
#include "Core/Components/SFW_LampControllerComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

static bool IsServer(const UWorld* W) {
    return W && W->GetNetMode() != NM_Client;
}

template<typename PredT>
static void ForEachLamp(UWorld* W, PredT Pred) {
    for (TActorIterator<AActor> It(W); It; ++It) {
        if (AActor* A = *It) {
            if (USFW_LampControllerComponent* L = A->FindComponentByClass<USFW_LampControllerComponent>()) {
                Pred(L);
            }
        }
    }
}

void USFW_PowerLibrary::BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds) {
    if (UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr) {
        if (!IsServer(W)) return; // server authority only
        ForEachLamp(W, [RoomId, Seconds](USFW_LampControllerComponent* L) {
            if (L && L->RoomId == RoomId) {
                L->SetState(ELampState::Off, Seconds);
            }
            });
    }
}

void USFW_PowerLibrary::BlackoutSite(UObject* WorldContextObject, float Seconds) {
    if (UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr) {
        if (!IsServer(W)) return; // server authority only
        ForEachLamp(W, [Seconds](USFW_LampControllerComponent* L) {
            if (L) {
                L->SetState(ELampState::Off, Seconds);
            }
            });
    }
}

void USFW_PowerLibrary::FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds)
{
    if (UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr) {
        if (!IsServer(W) || RoomId.IsNone()) return;
        ForEachLamp(W, [RoomId, Seconds](USFW_LampControllerComponent* L) {
            if (L && L->RoomId == RoomId)              // or L->GetRoomId()
                L->SetState(ELampState::Flicker, Seconds);
            });
    }
}
