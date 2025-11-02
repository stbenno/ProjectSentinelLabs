// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFW_PowerLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSFWPower, Log, All);

UCLASS()
class PROJECTSENTINELLABS_API USFW_PowerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 5.f);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void BlackoutSite(UObject* WorldContextObject, float Seconds = 5.f);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 3.f);
};
