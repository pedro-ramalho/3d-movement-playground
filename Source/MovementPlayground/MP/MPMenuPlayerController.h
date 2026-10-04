#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MPMenuPlayerController.generated.h"

class UMPMainMenuWidget;

UCLASS(Abstract)
class MOVEMENTPLAYGROUND_API AMPMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMPMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UMPMainMenuWidget> MainMenuWidget;
};
