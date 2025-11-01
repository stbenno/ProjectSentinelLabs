// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/Components/SFW_LampControllerComponent.h"
#include "SFW_LampBase.generated.h"

UCLASS()
class PROJECTSENTINELLABS_API ASFW_LampBase : public AActor
{
	GENERATED_BODY()

public:
	ASFW_LampBase();

	/** Logical room identifier to drive power/blackouts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lamp")
	FName RoomId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	UStaticMeshComponent* Mesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lamp")
	USFW_LampControllerComponent* Lamp = nullptr;

	/** Server-side helper to change state. */
	UFUNCTION(BlueprintCallable, Category = "Lamp")
	void SetLampState(ELampState NewState, float OptionalDurationSeconds = -1.f);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
