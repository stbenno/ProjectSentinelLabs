// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFW_AnomalyController.generated.h"

UCLASS()
class PROJECTSENTINELLABS_API ASFW_AnomalyController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASFW_AnomalyController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FTimerHandle TickHandle;
	
	//How fast to tick up (seconds)
	UPROPERTY(EditAnywhere, Category = "Anomaly")
	float TickInterval = 1.0f;

	//How much to add per tick
	UPROPERTY(EditAnywhere, Category = "Anomaly")
	float AggroPerTick = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Anomaly")
	float CollapseThreshold = 100.0f;

	void TickAggro();
};
