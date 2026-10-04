#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MPPauseMenuWidget.generated.h"

class AMovementPlaygroundPlayerController;
class UButton;

UCLASS(Abstract)
class MOVEMENTPLAYGROUND_API UMPPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnQuitClicked();

	[[nodiscard]] AMovementPlaygroundPlayerController* GetMPPlayerController() const;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSoftObjectPtr<UWorld> MainMenuLevel;
};
