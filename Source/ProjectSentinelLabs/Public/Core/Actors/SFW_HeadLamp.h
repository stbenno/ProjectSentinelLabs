// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreMinimal.h"
#include "Core/Actors/SFW_EquippableBase.h"
#include "SFW_HeadLamp.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class USoundBase;

UCLASS()
class PROJECTSENTINELLABS_API ASFW_HeadLamp : public ASFW_EquippableBase
{
	GENERATED_BODY()

public:
	ASFW_HeadLamp();

	virtual void OnEquipped(ACharacter* NewOwnerChar) override;
	virtual void OnUnequipped() override;

	UFUNCTION(BlueprintCallable, Category = "HeadLamp")
	void SetLampEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "HeadLamp")
	void ToggleLamp();

	UFUNCTION(BlueprintPure, Category = "HeadLamp")
	bool IsLampEnabled() const { return bLampEnabled; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeadLamp")
	TObjectPtr<UStaticMeshComponent> HeadlampMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HeadLamp")
	TObjectPtr<USpotLightComponent> Lamp;

	// Replicated lamp state
	UPROPERTY(ReplicatedUsing = OnRep_LampEnabled, BlueprintReadOnly, Category = "HeadLamp")
	bool bLampEnabled = false;

	UFUNCTION()
	void OnRep_LampEnabled();

	// Attach tuning
	UPROPERTY(EditDefaultsOnly, Category = "HeadLamp|Attach")
	FName HeadSocketName = TEXT("head_LampSocket");

	UPROPERTY(EditDefaultsOnly, Category = "HeadLamp|Attach")
	FVector AttachOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "HeadLamp|Attach")
	FRotator AttachRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "HeadLamp|Attach")
	FVector AttachScale = FVector(0.3f, 0.3f, 0.3f);

	// Toggle sound (played on all clients via OnRep)
	UPROPERTY(EditDefaultsOnly, Category = "HeadLamp|Audio")
	TObjectPtr<USoundBase> ToggleSound = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};