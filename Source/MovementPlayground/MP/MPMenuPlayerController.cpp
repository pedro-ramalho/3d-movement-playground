#include "MP/MPMenuPlayerController.h"

#include "MP/MPMainMenuWidget.h"

void AMPMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController() || !MainMenuWidgetClass)
	{
		return;
	}

	MainMenuWidget = CreateWidget<UMPMainMenuWidget>(this, MainMenuWidgetClass);

	if (!MainMenuWidget)
	{
		return;
	}

	MainMenuWidget->AddToPlayerScreen();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}
