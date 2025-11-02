// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFW_PowerLibrary.generated.h"

class USFW_LampControllerComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSFWPower, Log, All);

/** Site/room power effects for lamps. Safe to call from Blueprints. */
UCLASS()
class PROJECTSENTINELLABS_API USFW_PowerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Black out every lamp for Seconds. Server only has effect. */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void BlackoutSite(UObject* WorldContextObject, float Seconds = 5.f);

	/** Black out lamps with matching RoomId for Seconds. Server only has effect. */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 5.f);

	/** Flicker lamps with matching RoomId for Seconds. Server only has effect. */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 3.f);

private:
	static UWorld* GetWorldChecked(UObject* WorldContextObject);
	static bool IsServer(UWorld* World);

	static void ForEachLamp(UWorld* World, TFunctionRef<void(USFW_LampControllerComponent*)> Fn);
	static void ForEachLampInRoom(UWorld* World, FName RoomId, TFunctionRef<void(USFW_LampControllerComponent*)> Fn);
};

