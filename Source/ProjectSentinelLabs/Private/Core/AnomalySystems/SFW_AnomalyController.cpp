#include "Core/AnomalySystems/SFW_AnomalyController.h"
#include "Core/Game/SFW_GameState.h"
#include "Core/Rooms/RoomVolume.h"            // adjust include path if different
#include "Core/Lights/SFW_PowerLibrary.h"        // BlackoutRoom, FlickerRoom
#include "EngineUtils.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogAnomalyController);

ASFW_AnomalyController::ASFW_AnomalyController()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ASFW_AnomalyController::BeginPlay()
{
	Super::BeginPlay();
}

ASFW_GameState* ASFW_AnomalyController::GS() const
{
	return GetWorld() ? GetWorld()->GetGameState<ASFW_GameState>() : nullptr;
}

void ASFW_AnomalyController::StartRound()
{
	if (!HasAuthority()) return;

	PickRooms();

	if (ASFW_GameState* G = GS())
	{
		G->bRoundActive = true;
		G->RoundStartTime = GetWorld()->GetTimeSeconds();
		G->RoundSeed = FMath::Rand();
		G->BaseRoom = BaseRoom;
		G->RiftRoom = RiftRoom;
	}

	BuildDefaultActionTable();

	// 1 Hz loops are fine for now
	GetWorldTimerManager().SetTimer(DecisionHandle, this, &ASFW_AnomalyController::DecisionTick, 1.0f, true);
	GetWorldTimerManager().SetTimer(TickHandle, this, &ASFW_AnomalyController::ServerTick, 1.0f, true);

	UE_LOG(LogAnomalyController, Warning, TEXT("StartRound: AnomalyController Spawned."));
}

void ASFW_AnomalyController::PickRooms()
{
	BaseRoom = nullptr;
	RiftRoom = nullptr;

	UWorld* W = GetWorld();
	if (!W) return;

	// Prefer explicit typed rooms
	for (TActorIterator<ARoomVolume> It(W); It; ++It)
	{
		ARoomVolume* R = *It;
		if (!R) continue;

		if (!BaseRoom && R->RoomType == ERoomType::Base) { BaseRoom = R; continue; }
		if (!RiftRoom && R->RoomType == ERoomType::RiftCandidate) { RiftRoom = R; }
	}

	// Fallback: first two distinct rooms
	if (!BaseRoom || !RiftRoom)
	{
		for (TActorIterator<ARoomVolume> It2(W); It2; ++It2)
		{
			ARoomVolume* R = *It2;
			if (!R) continue;
			if (!BaseRoom) { BaseRoom = R; continue; }
			if (!RiftRoom && R != BaseRoom) { RiftRoom = R; break; }
		}
	}

	UE_LOG(LogAnomalyController, Log, TEXT("PickRooms: Base=%s Rift=%s"),
		*GetNameSafe(BaseRoom), *GetNameSafe(RiftRoom));
}

void ASFW_AnomalyController::BuildDefaultActionTable()
{
	// Placeholder so links resolve. Keep your real table here later.
}

void ASFW_AnomalyController::DecisionTick()
{
	if (!HasAuthority()) return;

	ASFW_GameState* G = GS();
	if (!G || !G->bRoundActive) return;

	if (RiftRoom)
	{
		const float Dur = FMath::FRandRange(2.0f, 6.0f);
		if (FMath::FRand() < 0.6f)
		{
			USFW_PowerLibrary::BlackoutRoom(this, RiftRoom->RoomId, Dur);
			UE_LOG(LogAnomalyController, Log, TEXT("Decision: BlackoutRiftRoom Room=%s Dur=%.2f"),
				*RiftRoom->RoomId.ToString(), Dur);
		}
		else
		{
			USFW_PowerLibrary::FlickerRoom(this, RiftRoom->RoomId, Dur);
			UE_LOG(LogAnomalyController, Log, TEXT("Decision: FlickerRiftRoom Room=%s Dur=%.2f"),
				*RiftRoom->RoomId.ToString(), Dur);
		}
	}
}

void ASFW_AnomalyController::ServerTick()
{
	if (!HasAuthority()) return;
	// Reserved for housekeeping.
}
