#pragma once

#include "CoreMinimal.h"
#include "MPMovementTypes.generated.h"

UENUM(BlueprintType)
enum class EMPCustomMovementMode : uint8
{
	None UMETA(Hidden),
	
	Slide UMETA(DisplayName="Slide"),
	
	WallRun UMETA(DisplayName="Wall Run"),
	
	Grapple UMETA(DisplayName="Grapple")
};

