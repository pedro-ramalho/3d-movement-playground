// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MPCourse.generated.h"

class AMPCheckpoint;

UENUM(BlueprintType)
enum class EMPRunState : uint8
{
	WaitingToStart UMETA(DisplayName="Waiting To Start"),

	Running UMETA(DisplayName="Running"),

	Finished UMETA(DisplayName="Finished")
};

UENUM(BlueprintType)
enum class EMPMedal : uint8
{
	None UMETA(DisplayName="None"),

	Bronze UMETA(DisplayName="Bronze"),

	Silver UMETA(DisplayName="Silver"),

	Gold UMETA(DisplayName="Gold"),

	Developer UMETA(DisplayName="Developer")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMPOnRunStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMPOnSplitRecorded, int32, SplitIndex, float, SplitTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMPOnRunFinished, float, FinalTime, EMPMedal, Medal, bool, bNewBest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMPOnRunReset);

UCLASS()
class MOVEMENTPLAYGROUND_API AMPCourse : public AActor
{
	GENERATED_BODY()

public:
	AMPCourse();

	UFUNCTION(BlueprintCallable, Category = "MP|Course")
	void ResetRun();

	UFUNCTION(BlueprintPure, Category = "MP|Course")
	EMPRunState GetRunState() const { return RunState; }

	UFUNCTION(BlueprintPure, Category = "MP|Course")
	float GetElapsedTime() const;

	UFUNCTION(BlueprintPure, Category = "MP|Course")
	int32 GetFinishSplitIndex() const { return Checkpoints.Num(); }

	UFUNCTION(BlueprintPure, Category = "MP|Course")
	bool TryGetBestSplit(int32 SplitIndex, float& OutSplitTime) const;

	UFUNCTION(BlueprintPure, Category = "MP|Course")
	FTransform GetRespawnTransform() const;

public:
	UPROPERTY(BlueprintAssignable, Category = "MP|Course")
	FMPOnRunStarted OnRunStarted;

	UPROPERTY(BlueprintAssignable, Category = "MP|Course")
	FMPOnSplitRecorded OnSplitRecorded;

	UPROPERTY(BlueprintAssignable, Category = "MP|Course")
	FMPOnRunFinished OnRunFinished;

	UPROPERTY(BlueprintAssignable, Category = "MP|Course")
	FMPOnRunReset OnRunReset;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnCheckpointReached(AMPCheckpoint* Checkpoint);

	void StartRun();

	void RecordSplit();

	void FinishRun();

	[[nodiscard]] EMPMedal GradeTime(float Time) const;

	[[nodiscard]] TArray<AMPCheckpoint*> GetAllCheckpoints() const;

private:
	UPROPERTY(EditInstanceOnly, Category = "MP|Course")
	TObjectPtr<AMPCheckpoint> StartLine;

	UPROPERTY(EditInstanceOnly, Category = "MP|Course")
	TArray<TObjectPtr<AMPCheckpoint>> Checkpoints;

	UPROPERTY(EditInstanceOnly, Category = "MP|Course")
	TObjectPtr<AMPCheckpoint> FinishLine;

	UPROPERTY(EditAnywhere, Category = "MP|Course|Medals", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float BronzeTime = 120.f;

	UPROPERTY(EditAnywhere, Category = "MP|Course|Medals", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float SilverTime = 90.f;

	UPROPERTY(EditAnywhere, Category = "MP|Course|Medals", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float GoldTime = 60.f;

	UPROPERTY(EditAnywhere, Category = "MP|Course|Medals", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float DeveloperTime = 45.f;

	EMPRunState RunState = EMPRunState::WaitingToStart;

	float RunStartTime = 0.f;

	TArray<float> CurrentSplits;

	TArray<float> BestSplits;
};
