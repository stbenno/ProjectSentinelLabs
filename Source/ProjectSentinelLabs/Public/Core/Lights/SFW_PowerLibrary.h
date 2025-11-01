// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFW_PowerLibrary.generated.h"

UCLASS()
class PROJECTSENTINELLABS_API USFW_PowerLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "SFW|Power", meta = (WorldContext = "WorldContextObject"))
	static void BlackoutRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 5.f);

	UFUNCTION(BlueprintCallable, Category = "SFW|Power", meta = (WorldContext = "WorldContextObject"))
	static void BlackoutSite(UObject* WorldContextObject, float Seconds = 5.f);

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"), Category = "SFW|Power")
	static void FlickerRoom(UObject* WorldContextObject, FName RoomId, float Seconds = 3.f);
};