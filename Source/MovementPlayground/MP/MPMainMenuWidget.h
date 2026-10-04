#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MPMainMenuWidget.generated.h"

class UButton;

UCLASS(Abstract)
class MOVEMENTPLAYGROUND_API UMPMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void OnQuitClicked();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	TSoftObjectPtr<UWorld> CourseLevel;
};
