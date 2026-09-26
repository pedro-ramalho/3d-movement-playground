#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "MPMovementTypes.generated.h"

UENUM(BlueprintType)
enum class EMPCustomMovementMode : uint8
{
	None UMETA(Hidden),
	
	Slide UMETA(DisplayName="Slide"),
	
	WallRun UMETA(DisplayName="Wall Run"),
	
	Grapple UMETA(DisplayName="Grapple")
};

constexpr ECollisionChannel ECC_WallRun = ECC_GameTraceChannel1;
