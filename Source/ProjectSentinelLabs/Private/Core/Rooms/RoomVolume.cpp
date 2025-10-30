// Fill out your copyright notice in the Description page of Project Settings.

// RoomVolume.cpp

#include "Core/Rooms/RoomVolume.h"
#include "Core/Game/SFW_GameState.h"
#include "Core/Game/SFW_PlayerState.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Components/ShapeComponent.h"

ARoomVolume::ARoomVolume()
{
    bNetLoadOnClient = true;

    // Overlap pawns only by default; adjust in BP if needed.
    GetCollisionComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GetCollisionComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
    GetCollisionComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ARoomVolume::BeginPlay()
{
    Super::BeginPlay();

    // sync safe flag from enum
    if (RoomType == ERoomType::Safe)
    {
        bIsSafeRoom = true;
    }

    OnActorBeginOverlap.AddDynamic(this, &ARoomVolume::HandleBeginOverlap);
    OnActorEndOverlap.AddDynamic(this, &ARoomVolume::HandleEndOverlap);
}

ASFW_GameState* ARoomVolume::GetSFWGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<ASFW_GameState>() : nullptr;
}

bool ARoomVolume::IsPlayerPawn(AActor* Actor) const
{
    const APawn* Pawn = Cast<APawn>(Actor);
    return Pawn && Pawn->IsPlayerControlled();
}

APlayerState* ARoomVolume::GetPlayerStateFromActor(AActor* Actor) const
{
    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        if (const AController* C = Pawn->GetController())
        {
            return C->PlayerState;
        }
    }
    return nullptr;
}

ASFW_PlayerState* ARoomVolume::GetSFWPlayerStateFromActor(AActor* Actor) const
{
    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        if (AController* C = Pawn->GetController())
        {
            return C->GetPlayerState<ASFW_PlayerState>();
        }
    }
    return nullptr;
}

bool ARoomVolume::ShouldProcess(AActor* Actor)
{
    const float Now = GetWorld()->TimeSeconds;
    float& LastTime = LastEventTime.FindOrAdd(Actor);
    if (Now - LastTime < DebounceSeconds) return false;
    LastTime = Now;
    return true;
}

void ARoomVolume::NotifyPresenceChanged(APlayerState* PS, bool bEnter) const
{
    UE_LOG(LogTemp, Log, TEXT("[RoomPresence] %s %s %s"),
        *GetNameSafe(PS),
        bEnter ? TEXT("ENTER") : TEXT("EXIT"),
        *RoomId.ToString());

    const ASFW_GameState* GS = GetSFWGameState();
    if (!GS) return;

    const bool bIsRift = (GS->RiftRoom == this);
    const bool bIsBase = (GS->BaseRoom == this);

    if (bIsRift)
    {
        UE_LOG(LogTemp, Log, TEXT("[RoomPresence] %s %s RIFT (%s)"),
            *GetNameSafe(PS),
            bEnter ? TEXT("ENTER") : TEXT("EXIT"),
            *RoomId.ToString());
    }
    if (bIsBase)
    {
        UE_LOG(LogTemp, Log, TEXT("[RoomPresence] %s %s BASE (%s)"),
            *GetNameSafe(PS),
            bEnter ? TEXT("ENTER") : TEXT("EXIT"),
            *RoomId.ToString());
    }
}

void ARoomVolume::UpdateSafeFlag(APawn* Pawn, int32 Delta)
{
    if (!Pawn) return;

    ASFW_PlayerState* PS = Pawn->GetPlayerState<ASFW_PlayerState>();
    if (!PS) return;

    int32& Count = SafeOverlapCount.FindOrAdd(PS);
    Count = FMath::Max(0, Count + Delta);

    if (PS->HasAuthority())
    {
        PS->SetInSafeRoom(Count > 0);
    }
}

void ARoomVolume::UpdateRiftFlag(APawn* Pawn, bool bEnter)
{
    if (!Pawn) return;

    ASFW_GameState* GS = GetSFWGameState();
    if (!GS) return;

    // Only the server should mark flags on PlayerState
    ASFW_PlayerState* PS = Pawn->GetPlayerState<ASFW_PlayerState>();
    if (!PS || !PS->HasAuthority()) return;

    const bool bThisIsRift = (GS->RiftRoom == this);

    if (!bThisIsRift)
    {
        // We only change the rift flag if this is actually the chosen RiftRoom.
        // If a player exits some random room that is not RiftRoom we don't touch bInRiftRoom here.
        if (!bEnter && PS->bInRiftRoom)
        {
            // special case:
            // If they are leaving the current RiftRoom (handled in HandleEndOverlap),
            // we will clear. That code path will still call UpdateRiftFlag with bEnter=false
            // on the same room they are exiting, so bThisIsRift will be true in that case.
        }
        return;
    }

    // We are in the actual RiftRoom
    PS->SetInRiftRoom(bEnter);
}

void ARoomVolume::HandleBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!HasAuthority() || !IsPlayerPawn(OtherActor) || !ShouldProcess(OtherActor)) return;

    APawn* Pawn = Cast<APawn>(OtherActor);
    APlayerState* GenericPS = GetPlayerStateFromActor(OtherActor);
    if (GenericPS)
    {
        NotifyPresenceChanged(GenericPS, /*bEnter*/ true);
    }

    // Safe-room enter
    if (bIsSafeRoom)
    {
        UpdateSafeFlag(Pawn, +1);
    }

    // Rift enter check
    UpdateRiftFlag(Pawn, /*bEnter*/ true);
}

void ARoomVolume::HandleEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    if (!HasAuthority() || !IsPlayerPawn(OtherActor) || !ShouldProcess(OtherActor)) return;

    APawn* Pawn = Cast<APawn>(OtherActor);
    APlayerState* GenericPS = GetPlayerStateFromActor(OtherActor);
    if (GenericPS)
    {
        NotifyPresenceChanged(GenericPS, /*bEnter*/ false);
    }

    // Safe-room exit
    if (bIsSafeRoom)
    {
        UpdateSafeFlag(Pawn, -1);
    }

    // Rift exit check
    UpdateRiftFlag(Pawn, /*bEnter*/ false);
}
