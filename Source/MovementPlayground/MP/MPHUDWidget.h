// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MP/MPCourse.h"
#include "MPHUDWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS()
class MOVEMENTPLAYGROUND_API UMPHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetCourse(AMPCourse* InCourse);

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void OnRunStarted();

	UFUNCTION()
	void OnSplitRecorded(int32 SplitIndex, float SplitTime);

	UFUNCTION()
	void OnRunFinished(float FinalTime, EMPMedal Medal, bool bNewBest);

	UFUNCTION()
	void OnRunReset();

	void HideSplit();

	[[nodiscard]] static FText FormatTime(float Seconds);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeedText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SplitText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ResultsPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> FinalTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MedalText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NewBestText;

	UPROPERTY(EditDefaultsOnly, Category = "MP|HUD", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float SplitDisplayDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|HUD")
	FLinearColor AheadColor = FLinearColor(0.2f, 0.9f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "MP|HUD")
	FLinearColor BehindColor = FLinearColor(0.95f, 0.25f, 0.2f);

	UPROPERTY(Transient)
	TObjectPtr<AMPCourse> Course;

	FTimerHandle SplitHideTimer;
};
