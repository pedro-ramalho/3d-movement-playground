#include "MP/MPPauseMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MovementPlaygroundPlayerController.h"

void UMPPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddDynamic(this, &UMPPauseMenuWidget::OnResumeClicked);
	RestartButton->OnClicked.AddDynamic(this, &UMPPauseMenuWidget::OnRestartClicked);
	MainMenuButton->OnClicked.AddDynamic(this, &UMPPauseMenuWidget::OnMainMenuClicked);
	QuitButton->OnClicked.AddDynamic(this, &UMPPauseMenuWidget::OnQuitClicked);
}

void UMPPauseMenuWidget::OnResumeClicked()
{
	if (AMovementPlaygroundPlayerController* PlayerController = GetMPPlayerController())
	{
		PlayerController->ResumeGame();
	}
}

void UMPPauseMenuWidget::OnRestartClicked()
{
	if (AMovementPlaygroundPlayerController* PlayerController = GetMPPlayerController())
	{
		PlayerController->ResumeGame();
		PlayerController->RestartRun();
	}
}

void UMPPauseMenuWidget::OnMainMenuClicked()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel);
}

void UMPPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

AMovementPlaygroundPlayerController* UMPPauseMenuWidget::GetMPPlayerController() const
{
	return GetOwningPlayer<AMovementPlaygroundPlayerController>();
}
