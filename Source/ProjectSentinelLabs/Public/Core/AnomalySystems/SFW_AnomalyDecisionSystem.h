#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/AnomalySystems/SFW_DecisionTypes.h"
#include "SFW_AnomalyDecisionSystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FSFWOnDecision, const FSFWDecisionPayload&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFWOnDecisionBP, const FSFWDecisionPayload&, Payload);

UCLASS()
class PROJECTSENTINELLABS_API ASFW_AnomalyDecisionSystem : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Decisions") UDataTable* DecisionsDT = nullptr;
	UPROPERTY(EditAnywhere, Category = "Decisions") float       IntervalSec = 3.f;

	FSFWOnDecision OnDecision;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDecision(const FSFWDecisionPayload& Payload);

	UPROPERTY(BlueprintAssignable, Category = "Decisions")
	FSFWOnDecisionBP OnDecisionBP;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle TickHandle;
	TMap<ESFWDecision, double> NextAllowedTime;

	void TickDecision(); // server-only
	bool IsReady(const FSFWDecisionRow& R) const;
	const FSFWDecisionRow* PickWeighted(int32 Tier);
	void StartCooldown(const FSFWDecisionRow& R);
	void Dispatch(const FSFWDecisionRow& R, FName RoomId);

	int32 GetRoomTier(FName RoomId) const; // returns 1..3
};