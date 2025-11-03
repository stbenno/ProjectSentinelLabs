#include "Core/AnomalySystems/SFW_AnomalyDecisionSystem.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Core/Rooms/RoomVolume.h"
#include "Core/Game/SFW_PlayerState.h"

// ---- Player-only room occupancy (ignores SafeRoom and similar) ----
static void GetOccupiedRooms(UWorld* World, TArray<FName>& Out)
{
	Out.Reset();
	if (!World) return;

	TSet<FName> Unique;

	for (TActorIterator<ARoomVolume> It(World); It; ++It)
	{
		ARoomVolume* Vol = *It;
		if (!Vol || Vol->RoomId.IsNone()) continue;
		if (Vol->bIsSafeRoom) continue; // do not target safe rooms

		TArray<AActor*> Over;
		Vol->GetOverlappingActors(Over, APawn::StaticClass());

		for (AActor* A : Over)
		{
			if (APawn* P = Cast<APawn>(A))
			{
				if (P->IsPlayerControlled())
				{
					Unique.Add(Vol->RoomId);
					break;
				}
			}
		}
	}

	Out = Unique.Array();
}

ASFW_AnomalyDecisionSystem::ASFW_AnomalyDecisionSystem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void ASFW_AnomalyDecisionSystem::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (!DecisionsDT)
		{
			UE_LOG(LogTemp, Warning, TEXT("AnomalyDecisionSystem: DecisionsDT is null."));
		}

		RefreshBiasFromProfile();

		UE_LOG(LogTemp, Warning,
			TEXT("AnomalyDecisionSystem: BeginPlay on server, IntervalSec=%.2f DoorBias=%.2f LightBias=%.2f"),
			IntervalSec, CachedDoorBias, CachedLightBias);

		GetWorldTimerManager().SetTimer(
			TickHandle,
			this,
			&ASFW_AnomalyDecisionSystem::TickDecision,
			IntervalSec,
			true
		);
	}
}

void ASFW_AnomalyDecisionSystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TickHandle);
	Super::EndPlay(EndPlayReason);
}

void ASFW_AnomalyDecisionSystem::RefreshBiasFromProfile()
{
	CachedDoorBias = 1.f;
	CachedLightBias = 1.f;

	if (!AnomalyProfilesDT)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[DecisionSystem] No AnomalyProfilesDT set; using neutral biases."));
		return;
	}

	for (const auto& Pair : AnomalyProfilesDT->GetRowMap())
	{
		if (FSFWAnomalyProfileRow* Row = reinterpret_cast<FSFWAnomalyProfileRow*>(Pair.Value))
		{
			if (Row->AnomalyType == ActiveAnomalyType)
			{
				CachedDoorBias = Row->DoorBias;
				CachedLightBias = Row->LightBias;

				UE_LOG(LogTemp, Warning,
					TEXT("[DecisionSystem] Using profile '%s' DoorBias=%.2f LightBias=%.2f"),
					*Row->DisplayName.ToString(), CachedDoorBias, CachedLightBias);
				return;
			}
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[DecisionSystem] No profile row for anomaly type %d; using neutral biases."),
		static_cast<int32>(ActiveAnomalyType));
}

bool ASFW_AnomalyDecisionSystem::IsReady(const FSFWDecisionRow& R) const
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (const double* T = NextAllowedTime.Find(R.Type))
	{
		return Now >= *T;
	}
	return true;
}

void ASFW_AnomalyDecisionSystem::StartCooldown(const FSFWDecisionRow& R)
{
	NextAllowedTime.FindOrAdd(R.Type) = GetWorld()->GetTimeSeconds() + R.CooldownSec;
}

const FSFWDecisionRow* ASFW_AnomalyDecisionSystem::PickWeighted(int32 Tier)
{
	if (!DecisionsDT) return nullptr;

	TArray<FSFWDecisionRow*> Options;
	for (const auto& Pair : DecisionsDT->GetRowMap())
	{
		if (FSFWDecisionRow* Row = reinterpret_cast<FSFWDecisionRow*>(Pair.Value))
		{
			if (Row->Tier == Tier && IsReady(*Row) && Row->Weight > 0.f)
			{
				Options.Add(Row);
			}
		}
	}

	if (Options.Num() == 0) return nullptr;

	auto GetBiasForDecision = [this](const FSFWDecisionRow* R) -> float
		{
			switch (R->Type)
			{
				// Light-related actions
			case ESFWDecision::LampFlicker:
			case ESFWDecision::BlackoutRoom:
				return CachedLightBias;

				// Door-related actions
			case ESFWDecision::OpenDoor:
			case ESFWDecision::CloseDoor:
			case ESFWDecision::LockDoor:
			case ESFWDecision::JamDoor:
			case ESFWDecision::KnockDoor:
				return CachedDoorBias;

			default:
				return 1.f; // neutral
			}
		};

	// Compute total biased weight
	float TotalW = 0.f;
	for (const FSFWDecisionRow* R : Options)
	{
		const float W = FMath::Max(0.f, R->Weight * GetBiasForDecision(R));
		TotalW += W;
	}

	if (TotalW <= 0.f) return nullptr;

	float Pick = FMath::FRandRange(0.f, TotalW);
	for (const FSFWDecisionRow* R : Options)
	{
		const float W = FMath::Max(0.f, R->Weight * GetBiasForDecision(R));
		Pick -= W;
		if (Pick <= 0.f)
		{
			return R;
		}
	}

	return Options.Last();
}

void ASFW_AnomalyDecisionSystem::Dispatch(const FSFWDecisionRow& R, FName RoomId)
{
	FSFWDecisionPayload P{ R.Type, RoomId, R.Magnitude, R.Duration, this };

	// Bridge decisions to existing systems on the server
	if (HasAuthority())
	{
		switch (R.Type)
		{
		case ESFWDecision::LampFlicker:
			USFW_PowerLibrary::FlickerRoom(this, RoomId, R.Duration);
			break;

		case ESFWDecision::BlackoutRoom:
			USFW_PowerLibrary::BlackoutRoom(this, RoomId, R.Duration);
			break;

		default:
			break; // doors, jumpscares, etc are handled by their own listeners
		}
	}

	// Notify generic listeners (doors, future systems, BP)
	OnDecision.Broadcast(P);
	MulticastDecision(P);
}

void ASFW_AnomalyDecisionSystem::MulticastDecision_Implementation(const FSFWDecisionPayload& Payload)
{
	OnDecisionBP.Broadcast(Payload);
}

// local helper
static int32 SanityTierToInt(ESanityTier T)
{
	switch (T)
	{
	case ESanityTier::T1: return 1;
	case ESanityTier::T2: return 2;
	case ESanityTier::T3: return 3;
	default:              return 1;
	}
}

int32 ASFW_AnomalyDecisionSystem::GetRoomTier(FName RoomId) const
{
	int32 MaxTier = 1;

	for (TActorIterator<ARoomVolume> It(GetWorld()); It; ++It)
	{
		ARoomVolume* Vol = *It;
		if (!Vol || Vol->RoomId != RoomId) continue;

		TArray<AActor*> Over;
		Vol->GetOverlappingActors(Over, APawn::StaticClass());

		for (AActor* A : Over)
		{
			APawn* P = Cast<APawn>(A);
			if (!P || !P->IsPlayerControlled()) continue;

			if (ASFW_PlayerState* PS = P->GetPlayerState<ASFW_PlayerState>())
			{
				MaxTier = FMath::Max(MaxTier, SanityTierToInt(PS->GetSanityTier()));
			}
		}
		break;
	}

	return MaxTier;
}

void ASFW_AnomalyDecisionSystem::TickDecision()
{
	if (!HasAuthority() || !DecisionsDT) return;

	TArray<FName> CandidateRooms;
	GetOccupiedRooms(GetWorld(), CandidateRooms);

	UE_LOG(LogTemp, Warning, TEXT("[DecisionSystem] TickDecision: Candidates=%d"), CandidateRooms.Num());

	if (CandidateRooms.Num() == 0) return;

	const FName Room = CandidateRooms[FMath::RandRange(0, CandidateRooms.Num() - 1)];
	const int32 Tier = GetRoomTier(Room);

	if (const FSFWDecisionRow* R = PickWeighted(Tier))
	{
		StartCooldown(*R);
		Dispatch(*R, Room);
	}
}
