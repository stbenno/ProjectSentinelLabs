#pragma once


#include "CoreMinimal.h"
#include "SFW_EquipmentTypes.generated.h"

UENUM(BlueprintType)
enum class EHeldItemType : uint8
{
    None        UMETA(DisplayName = "None"),
    Flashlight  UMETA(DisplayName = "Flashlight"),
    Camera      UMETA(DisplayName = "Camera")
};

UENUM(BlueprintType)
enum class EEquipState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Equipping   UMETA(DisplayName = "Equipping"),
    Holding     UMETA(DisplayName = "Holding"),
    Using       UMETA(DisplayName = "Using"),
    Unequipping UMETA(DisplayName = "Unequipping")
};