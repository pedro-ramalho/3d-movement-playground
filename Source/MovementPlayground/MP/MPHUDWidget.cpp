#include "MP/MPHUDWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UMPHUDWidget::SetCourse(AMPCourse* InCourse)
{
	Course = InCourse;
}

void UMPHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SplitText->SetVisibility(ESlateVisibility::Collapsed);
	ResultsPanel->SetVisibility(ESlateVisibility::Collapsed);
	
	if (Course)
	{
		Course->OnRunStarted.AddDynamic(this, &UMPHUDWidget::OnRunStarted);
		Course->OnRunReset.AddDynamic(this, &UMPHUDWidget::OnRunReset);
		Course->OnSplitRecorded.AddDynamic(this, &UMPHUDWidget::OnSplitRecorded);
		Course->OnRunFinished.AddDynamic(this, &UMPHUDWidget::OnRunFinished);
	}
}

void UMPHUDWidget::NativeDestruct()
{
	if (Course)
	{
		Course->OnRunStarted.RemoveDynamic(this, &UMPHUDWidget::OnRunStarted);
		Course->OnRunReset.RemoveDynamic(this, &UMPHUDWidget::OnRunReset);
		Course->OnSplitRecorded.RemoveDynamic(this, &UMPHUDWidget::OnSplitRecorded);
		Course->OnRunFinished.RemoveDynamic(this, &UMPHUDWidget::OnRunFinished);
	}
	
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SplitHideTimer);
	}
	
	Super::NativeDestruct();
}

void UMPHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (Course)
	{
		TimerText->SetText(FormatTime(Course->GetElapsedTime()));
	}
	
	if (const APawn* Pawn = GetOwningPlayerPawn())
	{
		const int32 Speed = FMath::RoundToInt(Pawn->GetVelocity().Size2D() * 0.036f);
		SpeedText->SetText(FText::FromString(FString::Printf(TEXT("%d km/h"), Speed)));
	}
}

void UMPHUDWidget::OnRunStarted()
{
	ResultsPanel->SetVisibility(ESlateVisibility::Collapsed);
	SplitText->SetVisibility(ESlateVisibility::Collapsed);
}

void UMPHUDWidget::OnSplitRecorded(int32 SplitIndex, float SplitTime)
{
	float BestTime = 0.f;

	if (Course->TryGetBestSplit(SplitIndex, BestTime))
	{
		const float Delta = SplitTime - BestTime;
		
		SplitText->SetColorAndOpacity(FSlateColor(Delta > 0.f ? BehindColor : AheadColor));
		SplitText->SetText(FText::FromString(FString::Printf(TEXT("%+.3f"), Delta)));
	}
	else
	{
		SplitText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		SplitText->SetText(FormatTime(SplitTime));
	}
	
	SplitText->SetVisibility(ESlateVisibility::HitTestInvisible);
	
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SplitHideTimer, this, &UMPHUDWidget::HideSplit, SplitDisplayDuration, false);
	}
}

void UMPHUDWidget::OnRunFinished(float FinalTime, EMPMedal Medal, bool bNewBest)
{
	FinalTimeText->SetText(FormatTime(FinalTime));
	MedalText->SetText(UEnum::GetDisplayValueAsText(Medal));
	NewBestText->SetVisibility(bNewBest ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	ResultsPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMPHUDWidget::OnRunReset()
{
	OnRunStarted();
}

void UMPHUDWidget::HideSplit()
{
	SplitText->SetVisibility(ESlateVisibility::Collapsed);
}

FText UMPHUDWidget::FormatTime(float Seconds)
{
	const int32 TotalMs = FMath::FloorToInt(FMath::Max(Seconds, 0.f) * 1000.f);
	const int32 Minutes = TotalMs / 60000;
	const int32 Secs = (TotalMs % 60000) / 1000;
	const int32 Ms = TotalMs % 1000;
	
	return FText::FromString(FString::Printf(TEXT("%d:%02d.%03d"), Minutes, Secs, Ms));
}
