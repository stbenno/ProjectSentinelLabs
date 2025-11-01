// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/AnomalySystems/SFW_AnomalyController.h"
#include "Core/AnomalySystems/SFW_AnomalyDecisionSystem.h"
#include "Core/Game/SFW_GameState.h"
#include "Core/Game/SFW_PlayerState.h"
#include "Core/Rooms/RoomVolume.h"
#include "Core/Actors/SFW_DoorBase.h"

#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

#include "Core/Lights/SFW_PowerLibrary.h"
#include "Core/AnomalySystems/SFW_DecisionTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnomalyController, Log, All);

const FName ASFW_AnomalyController::TAG_BaseRoom(TEXT("BaseRoomCandidate"));
const FName ASFW_AnomalyController::TAG_RiftRoom(TEXT("RiftRoomCandidate"));
const FName ASFW_AnomalyController::TAG_ShadeSpawn(TEXT("ShadeSpawn"));
const FName ASFW_AnomalyController::TAG_ShadeActor(TEXT("Shade"));

ASFW_AnomalyController::ASFW_AnomalyController()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void ASFW_AnomalyController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// bind to decision broadcaster
		for (TActorIterator<ASFW_AnomalyDecisionSystem> It(GetWorld()); It; ++It)
		{
			It->OnDecision.AddUObject(this, &ASFW_AnomalyController::HandleDecision);
			break;
		}

		CacheSafeRooms();
		StartRound();
	}
}

void ASFW_AnomalyController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority())
	{
		GetWorldTimerManager().ClearTimer(DecisionHandle);
		GetWorldTimerManager().ClearTimer(TickHandle);
	}
	Super::EndPlay(Reason);
}

bool ASFW_AnomalyController::AnyPlayerInRift() const
{
	const ASFW_GameState* G = GS();
	if (!G || !G->RiftRoom) return false;

	for (APlayerState* APS : G->PlayerArray)
	{
		const ASFW_PlayerState* P = Cast<ASFW_PlayerState>(APS);
		if (P && P->bInRiftRoom)
		{
			return true;
		}
	}
	return false;
}

void ASFW_AnomalyController::StartRound()
{
	if (!HasAuthority()) return;

	PickRooms();

	if (ASFW_GameState* G = GS())
	{
		G->RoundStartTime = GetWorld()->GetTimeSeconds();
		G->RoundSeed = FMath::Rand();

		// make sure GameState knows chosen two key rooms
		G->BaseRoom = BaseRoom;
		G->RiftRoom = RiftRoom;
	}

	BuildDefaultActionTable();

	// 1 Hz director tick
	GetWorldTimerManager().SetTimer(DecisionHandle, this, &ASFW_AnomalyController::DecisionTick, 1.0f, true);

	// future housekeeping (sanity global drift not handled here)
	GetWorldTimerManager().SetTimer(TickHandle, this, &ASFW_AnomalyController::ServerTick, 1.0f, true);
}

ASFW_GameState* ASFW_AnomalyController::GS() const
{
	return GetWorld() ? GetWorld()->GetGameState<ASFW_GameState>() : nullptr;
}

// ------------------------
// Room picking
// ------------------------
static bool IsEligibleRoomVolume(ARoomVolume* V)
{
	if (!V) return false;
	if (V->bIsSafeRoom) return false;
	// Block Safe/Hallway from being chosen as Base/Rift
	if (V->RoomType == ERoomType::Safe) return false;
	if (V->RoomType == ERoomType::Hallway) return false;
	return true;
}

void ASFW_AnomalyController::PickRooms()
{
	// ---- Base candidates ----
	TArray<AActor*> BaseRaw;
	UGameplayStatics::GetAllActorsWithTag(this, TAG_BaseRoom, BaseRaw);

	TArray<ARoomVolume*> BaseCands;
	for (AActor* A : BaseRaw)
		if (ARoomVolume* V = Cast<ARoomVolume>(A))
			if (IsEligibleRoomVolume(V)) BaseCands.Add(V);

	if (BaseCands.Num() == 0)
	{
		UE_LOG(LogAnomalyController, Warning, TEXT("No eligible BaseRoomCandidate actors found."));
		return;
	}

	const int32 BaseIdx = FMath::RandRange(0, BaseCands.Num() - 1);
	ARoomVolume* BaseV = BaseCands[BaseIdx];
	BaseRoom = BaseV;

	// ---- Rift candidates ----
	TArray<AActor*> RiftRaw;
	UGameplayStatics::GetAllActorsWithTag(this, TAG_RiftRoom, RiftRaw);

	TArray<ARoomVolume*> RiftCands;
	for (AActor* A : RiftRaw)
		if (ARoomVolume* V = Cast<ARoomVolume>(A))
			if (V != BaseV && IsEligibleRoomVolume(V)) RiftCands.Add(V);

	// If nothing in Rift tag list, try Base list again as a fallback pool (still filtered)
	if (RiftCands.Num() == 0)
	{
		for (ARoomVolume* V : BaseCands)
			if (V != BaseV && IsEligibleRoomVolume(V)) RiftCands.AddUnique(V);
	}

	if (RiftCands.Num() == 0)
	{
		UE_LOG(LogAnomalyController, Warning, TEXT("No eligible RiftRoomCandidate available after excluding Base/Safe/Hallway."));
		return;
	}

	// weight by path distance (closer = higher weight)
	TArray<float> Weights; Weights.Reserve(RiftCands.Num());
	for (ARoomVolume* R : RiftCands)
	{
		const float d = PathDistanceUU(BaseV, R);
		const float sigma = FMath::Max(1.f, RiftSigmaUU);
		const float w = FMath::Exp(-d / sigma); // closer gets higher weight
		Weights.Add(FMath::Max(0.0001f, w));
	}

	float Total = 0.f; for (float w : Weights) Total += w;
	float r = FMath::FRandRange(0.f, Total);
	int32 Pick = 0;
	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		if (r <= Weights[i]) { Pick = i; break; }
		r -= Weights[i];
	}
	RiftRoom = RiftCands[Pick];

	UE_LOG(LogAnomalyController, Log, TEXT("Base=%s Rift=%s"),
		BaseRoom ? *BaseRoom->GetName() : TEXT("None"),
		RiftRoom ? *RiftRoom->GetName() : TEXT("None"));
}

float ASFW_AnomalyController::PathDistanceUU(const AActor* A, const AActor* B) const
{
	if (!A || !B) return 0.f;

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return FVector::Dist(A->GetActorLocation(), B->GetActorLocation());
	}

	const UNavigationPath* Path =
		NavSys->FindPathToLocationSynchronously(GetWorld(), A->GetActorLocation(), B->GetActorLocation());

	if (!Path || Path->PathPoints.Num() < 2)
	{
		return FVector::Dist(A->GetActorLocation(), B->GetActorLocation());
	}

	double Sum = 0.0;
	for (int32 i = 1; i < Path->PathPoints.Num(); ++i)
	{
		Sum += FVector::Dist(Path->PathPoints[i - 1], Path->PathPoints[i]);
	}
	return static_cast<float>(Sum);
}

float ASFW_AnomalyController::LowestSanityInRift() const
{
	float Lowest = 100.f;
	const ASFW_GameState* G = GS();
	if (!G || !G->RiftRoom) return Lowest;

	for (APlayerState* APS : G->PlayerArray)
	{
		const ASFW_PlayerState* P = Cast<ASFW_PlayerState>(APS);
		if (P && P->bInRiftRoom)
		{
			Lowest = FMath::Min(Lowest, P->Sanity);
		}
	}
	return Lowest;
}

// ------------------------
// Shade helpers
// ------------------------
int32 ASFW_AnomalyController::CountActiveShades() const
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(this, TAG_ShadeActor, Found);

	int32 Alive = 0;
	for (AActor* A : Found)
	{
		if (IsValid(A) && !A->IsActorBeingDestroyed())
		{
			++Alive;
		}
	}
	return Alive;
}

void ASFW_AnomalyController::EnsureBaselineShade()
{
	if (!HasAuthority()) return;
	if (!RiftRoom) return;
	if (CountActiveShades() >= 1) return;
	if (!ShadePawnClass)
	{
		UE_LOG(LogAnomalyController, Warning, TEXT("ShadePawnClass not set."));
		return;
	}

	FVector Center = RiftRoom->GetActorLocation();
	float Radius = 600.f;

	if (const UBoxComponent* Box = Cast<AActor>(RiftRoom)->FindComponentByClass<UBoxComponent>())
	{
		Center = Box->GetComponentLocation();
		Radius = Box->GetScaledBoxExtent().GetMax() * 0.8f;
	}

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FVector SpawnAt = Center + FVector(0, 0, 30.f);
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->GetRandomReachablePointInRadius(Center, Radius, NavLoc))
		{
			SpawnAt = NavLoc.Location + FVector(0, 0, 2.f);
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* Shade = GetWorld()->SpawnActor<APawn>(ShadePawnClass, SpawnAt, FRotator::ZeroRotator, Params);
	if (Shade)
	{
		Shade->Tags.AddUnique(TAG_ShadeActor);
		Shade->SpawnDefaultController();
		UE_LOG(LogAnomalyController, Log, TEXT("Baseline Shade spawned at %s"), *SpawnAt.ToCompactString());
	}
}

void ASFW_AnomalyController::TryPunishShade()
{
	UE_LOG(LogAnomalyController, Verbose, TEXT("TryPunishShade() placeholder"));
}

// ------------------------
// Evidence helpers
// ------------------------
int32 ASFW_AnomalyController::PickEvidenceTypeForClass(EAnomalyClass /*Class*/) const
{
	// map anomaly class to its 3 allowed evidence types (internal numeric IDs)
	// For now just always EMF = 1 so we compile and test
	return 1;
}

// ------------------------
// Action table / decision loop
// ------------------------
void ASFW_AnomalyController::BuildDefaultActionTable()
{
	ActionTable.Empty();

	// 85-100 sanity: mostly chill. do nothing.
	{
		FAnomalyActionRow Row;
		Row.ActionId = EAnomalyActionId::DoNothing;
		Row.SanityMin = 85.f;
		Row.SanityMax = 100.f;
		Row.bRequiresInRift = true;
		Row.MinShades = 0;
		Row.MaxShades = 99;
		Row.GlobalCooldownSec = 6.f;
		Row.Weight = 4.f;
		ActionTable.Add(Row);
	}

	// <85 sanity: allowed to spawn baseline shade if none alive
	{
		FAnomalyActionRow Row;
		Row.ActionId = EAnomalyActionId::SpawnBaselineShade;
		Row.SanityMin = 0.f;
		Row.SanityMax = 85.f;
		Row.bRequiresInRift = true;
		Row.MinShades = 0;
		Row.MaxShades = 0; // only when zero shades alive
		Row.GlobalCooldownSec = 5.f;
		Row.Weight = 10.f;
		ActionTable.Add(Row);
	}

	// evidence window opening opportunity
	{
		FAnomalyActionRow Row;
		Row.ActionId = EAnomalyActionId::OpenEvidenceWindow;
		Row.SanityMin = EvidenceSanityMin;
		Row.SanityMax = EvidenceSanityMax;
		Row.bRequiresInRift = true;
		Row.MinShades = 0;
		Row.MaxShades = 99;
		Row.GlobalCooldownSec = EvidenceWindowCooldownSec;
		Row.Weight = 3.f;
		ActionTable.Add(Row);
	}
}

bool ASFW_AnomalyController::ActionOnCooldown(EAnomalyActionId Id, float Now) const
{
	const uint8 Key = static_cast<uint8>(Id);
	const float* Last = LastFiredByAction.Find(Key);
	if (!Last) return false;

	for (const FAnomalyActionRow& R : ActionTable)
	{
		if (R.ActionId == Id)
		{
			return (Now - *Last) < R.GlobalCooldownSec;
		}
	}
	return false;
}

void ASFW_AnomalyController::MarkActionFired(EAnomalyActionId Id, float Now)
{
	LastFiredByAction.FindOrAdd(static_cast<uint8>(Id)) = Now;
}

const FAnomalyActionRow* ASFW_AnomalyController::ChooseAction(float Now, float Lowest, bool bInRift, int32 ShadesAlive) const
{
	TArray<const FAnomalyActionRow*> Eligible;
	TArray<float> Weights;

	for (const FAnomalyActionRow& R : ActionTable)
	{
		if (Lowest < R.SanityMin || Lowest > R.SanityMax) continue;
		if (R.bRequiresInRift && !bInRift) continue;
		if (ShadesAlive < R.MinShades || ShadesAlive > R.MaxShades) continue;
		if (R.Weight <= 0.f) continue;

		// per-action cooldown
		if (LastFiredByAction.Contains(static_cast<uint8>(R.ActionId)))
		{
			const float Last = LastFiredByAction[static_cast<uint8>(R.ActionId)];
			if ((Now - Last) < R.GlobalCooldownSec) continue;
		}

		Eligible.Add(&R);
		Weights.Add(R.Weight);
	}

	if (Eligible.Num() == 0) return nullptr;

	float Total = 0.f;
	for (float W : Weights) Total += W;

	float r = FMath::FRandRange(0.f, Total);
	for (int32 i = 0; i < Eligible.Num(); ++i)
	{
		if (r <= Weights[i]) return Eligible[i];
		r -= Weights[i];
	}
	return Eligible.Last();
}

void ASFW_AnomalyController::ExecuteAction(const FAnomalyActionRow& Row, float Now, float Lowest, bool bInRift, int32 ShadesAlive)
{
	switch (Row.ActionId)
	{
	case EAnomalyActionId::DoNothing:
		UE_LOG(LogAnomalyController, Verbose, TEXT("Decision: DoNothing"));
		MarkActionFired(Row.ActionId, Now);
		break;

	case EAnomalyActionId::SpawnBaselineShade:
		if (Lowest <= SpawnGateSanity && ShadesAlive < 1 && bInRift)
		{
			EnsureBaselineShade();
			MarkActionFired(Row.ActionId, Now);
			UE_LOG(LogAnomalyController, Log, TEXT("Decision: SpawnBaselineShade"));
		}
		break;

	case EAnomalyActionId::OpenEvidenceWindow:
	{
		ASFW_GameState* G = GS(); if (!G) break;

		if (G->bEvidenceWindowActive)
		{
			const float SinceStart = Now - G->EvidenceWindowStartTime;
			if (SinceStart >= G->EvidenceWindowDurationSec)
			{
				G->Server_EndEvidenceWindow();
				UE_LOG(LogAnomalyController, Log, TEXT("EvidenceWindow END"));
			}
			break;
		}

		const bool InBand = (Lowest >= EvidenceSanityMin && Lowest <= EvidenceSanityMax);
		const float SinceLast = Now - LastEvidenceFireTime;
		const bool CooldownOK = (SinceLast >= EvidenceWindowCooldownSec);
		const bool FailsafeOK = (SinceLast >= EvidenceWindowFailsafeSec);

		if (bInRift && ((InBand && CooldownOK) || FailsafeOK))
		{
			const int32 EvidenceType = PickEvidenceTypeForClass(G->ActiveClass);
			G->Server_StartEvidenceWindow(EvidenceType, EvidenceWindowDurationSec);
			LastEvidenceFireTime = Now;
			MarkActionFired(Row.ActionId, Now);
			UE_LOG(LogAnomalyController, Log, TEXT("EvidenceWindow START Class=%d EvidenceType=%d"),
				static_cast<int32>(G->ActiveClass), EvidenceType);
		}
		break;
	}

	default: break;
	}
}

void ASFW_AnomalyController::DecisionTick()
{
	if (!HasAuthority()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	const bool bInRift = AnyPlayerInRift();
	const float Lowest = LowestSanityInRift();
	const int32 Shades = CountActiveShades();

	// Maintain evidence timers even if no one currently in rift
	if (ASFW_GameState* G = GS())
	{
		if (G->bEvidenceWindowActive)
		{
			const float SinceStart = Now - G->EvidenceWindowStartTime;
			if (SinceStart >= G->EvidenceWindowDurationSec)
			{
				G->Server_EndEvidenceWindow();
				UE_LOG(LogAnomalyController, Log, TEXT("EvidenceWindow END (maintenance)"));
			}
		}
	}

	// If nobody is in rift, skip aggressive logic
	if (!bInRift)
	{
		return;
	}

	const FAnomalyActionRow* Pick = ChooseAction(Now, Lowest, bInRift, Shades);
	if (!Pick)
	{
		// tag DoNothing to prevent thrash
		MarkActionFired(EAnomalyActionId::DoNothing, Now);
		return;
	}

	ExecuteAction(*Pick, Now, Lowest, bInRift, Shades);
}

void ASFW_AnomalyController::ServerTick()
{
	if (!HasAuthority()) return;

	// placeholder for global pacing, punish shade, etc
	TryPunishShade();
}

// ------------------------
// Safe-room cache and decision routing
// ------------------------
void ASFW_AnomalyController::CacheSafeRooms()
{
	SafeRoomIds.Reset();

	for (TActorIterator<ARoomVolume> It(GetWorld()); It; ++It)
	{
		ARoomVolume* V = *It;
		if (!V || V->RoomId.IsNone()) continue;

		if (V->Tags.Contains(FName("SafeRoom")) || V->bIsSafeRoom || V->RoomType == ERoomType::Safe)
		{
			SafeRoomIds.Add(V->RoomId);
		}
	}
}

void ASFW_AnomalyController::HandleDecision(const FSFWDecisionPayload& P)
{
	if (!HasAuthority()) return;
	if (IsSafeRoom(P.RoomId)) return; // never act in safe room

	switch (P.Type)
	{
	case ESFWDecision::OpenDoor:
	case ESFWDecision::CloseDoor:
	case ESFWDecision::LockDoor:
	case ESFWDecision::JamDoor:
	{
		for (TActorIterator<ASFW_DoorBase> It(GetWorld()); It; ++It)
		{
			if (ASFW_DoorBase* Door = *It)
			{
				// Optional: filter by room if your doors expose RoomId
				// if (Door->GetRoomID() != P.RoomId) continue;
				Door->HandleDecision(P);
			}
		}
		break;
	}

	case ESFWDecision::LampFlicker:
		if (!P.RoomId.IsNone())
		{
			USFW_PowerLibrary::FlickerRoom(this, P.RoomId, P.Duration);
			BP_HandleLampFlicker(P); // optional BP VFX hook
		}
		break;

	case ESFWDecision::BlackoutRoom:
		if (!P.RoomId.IsNone())
		{
			USFW_PowerLibrary::BlackoutRoom(this, P.RoomId, P.Duration);
		}
		break;

	default:
		break;
	}
}
