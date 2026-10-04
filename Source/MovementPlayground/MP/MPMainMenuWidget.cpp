#include "MP/MPMainMenuWidget.h"

#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMPMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UMPMainMenuWidget::OnPlayClicked);
	QuitButton->OnClicked.AddDynamic(this, &UMPMainMenuWidget::OnQuitClicked);
}

void UMPMainMenuWidget::OnPlayClicked()
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, CourseLevel);
}

void UMPMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
