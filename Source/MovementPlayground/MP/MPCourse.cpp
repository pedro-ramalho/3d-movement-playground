#include "MP/MPCourse.h"
#include "MP/MPCheckpoint.h"
#include "MP/MPSaveGame.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMPCourse, Log, All);

AMPCourse::AMPCourse()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMPCourse::ResetRun()
{
	RunState = EMPRunState::WaitingToStart;
	RunStartTime = 0.f;
	CurrentSplits.Reset();

	OnRunReset.Broadcast();
}

float AMPCourse::GetElapsedTime() const
{
	switch (RunState)
	{
	case EMPRunState::Running:
		return GetWorld()->GetTimeSeconds() - RunStartTime;

	case EMPRunState::Finished:
		return CurrentSplits.Last();

	default:
		return 0.f;
	}
}

bool AMPCourse::TryGetBestSplit(int32 SplitIndex, float& OutSplitTime) const
{
	if (!BestSplits.IsValidIndex(SplitIndex))
	{
		return false;
	}

	OutSplitTime = BestSplits[SplitIndex];
	return true;
}

FTransform AMPCourse::GetRespawnTransform() const
{
	const int32 LastReachedIndex = FMath::Min(CurrentSplits.Num(), Checkpoints.Num()) - 1;

	if (Checkpoints.IsValidIndex(LastReachedIndex) && Checkpoints[LastReachedIndex])
	{
		return Checkpoints[LastReachedIndex]->GetSpawnTransform();
	}

	return StartLine ? StartLine->GetSpawnTransform() : GetActorTransform();
}

void AMPCourse::BeginPlay()
{
	Super::BeginPlay();

	if (!StartLine || !FinishLine)
	{
		UE_LOG(LogMPCourse, Warning, TEXT("%s is missing a start or finish line"), *GetName());
	}

	for (AMPCheckpoint* Checkpoint : GetAllCheckpoints())
	{
		if (Checkpoint)
		{
			Checkpoint->OnPlayerReached.AddUObject(this, &AMPCourse::OnCheckpointReached);
		}
	}
	
	LoadBestRun();
}

void AMPCourse::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AMPCheckpoint* Checkpoint : GetAllCheckpoints())
	{
		if (Checkpoint)
		{
			Checkpoint->OnPlayerReached.RemoveAll(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AMPCourse::OnCheckpointReached(AMPCheckpoint* Checkpoint)
{
	if (Checkpoint == StartLine)
	{
		if (RunState != EMPRunState::Running)
		{
			StartRun();
		}

		return;
	}

	if (RunState != EMPRunState::Running)
	{
		return;
	}

	const int32 NextIndex = CurrentSplits.Num();

	if (Checkpoint == FinishLine)
	{
		if (NextIndex == Checkpoints.Num())
		{
			FinishRun();
		}

		return;
	}

	if (Checkpoints.IsValidIndex(NextIndex) && Checkpoints[NextIndex] == Checkpoint)
	{
		RecordSplit();
	}
}

void AMPCourse::StartRun()
{
	RunState = EMPRunState::Running;
	RunStartTime = GetWorld()->GetTimeSeconds();
	CurrentSplits.Reset();

	UE_LOG(LogMPCourse, Log, TEXT("Run started"));

	OnRunStarted.Broadcast();
}

void AMPCourse::RecordSplit()
{
	const float SplitTime = GetElapsedTime();
	const int32 SplitIndex = CurrentSplits.Add(SplitTime);

	UE_LOG(LogMPCourse, Log, TEXT("Split %d: %.3f s"), SplitIndex, SplitTime);

	OnSplitRecorded.Broadcast(SplitIndex, SplitTime);
}

void AMPCourse::FinishRun()
{
	RecordSplit();

	RunState = EMPRunState::Finished;

	const float FinalTime = CurrentSplits.Last();
	const EMPMedal Medal = GradeTime(FinalTime);
	const bool bNewBest = BestSplits.IsEmpty() || FinalTime < BestSplits.Last();

	if (bNewBest)
	{
		BestSplits = CurrentSplits;
		SaveBestRun();
	}

	UE_LOG(LogMPCourse, Log, TEXT("Run finished: %.3f s, medal: %s, new best: %s"),
		FinalTime, *UEnum::GetDisplayValueAsText(Medal).ToString(), bNewBest ? TEXT("yes") : TEXT("no"));

	OnRunFinished.Broadcast(FinalTime, Medal, bNewBest);
}

EMPMedal AMPCourse::GradeTime(float Time) const
{
	if (Time <= DeveloperTime)
	{
		return EMPMedal::Developer;
	}

	if (Time <= GoldTime)
	{
		return EMPMedal::Gold;
	}

	if (Time <= SilverTime)
	{
		return EMPMedal::Silver;
	}

	if (Time <= BronzeTime)
	{
		return EMPMedal::Bronze;
	}

	return EMPMedal::None;
}

TArray<AMPCheckpoint*> AMPCourse::GetAllCheckpoints() const
{
	TArray<AMPCheckpoint*> AllCheckpoints;
	AllCheckpoints.Reserve(Checkpoints.Num() + 2);

	AllCheckpoints.Add(StartLine);

	for (const TObjectPtr<AMPCheckpoint>& Checkpoint : Checkpoints)
	{
		AllCheckpoints.Add(Checkpoint);
	}

	AllCheckpoints.Add(FinishLine);

	return AllCheckpoints;
}

void AMPCourse::LoadBestRun()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		return;
	}
	
	if (const UMPSaveGame* Save = Cast<UMPSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
	{
		if (Save->BestSplits.Num() == Checkpoints.Num() + 1)
		{
			BestSplits = Save->BestSplits;
		}
	}
}

void AMPCourse::SaveBestRun() const
{
	if (UMPSaveGame* Save = Cast<UMPSaveGame>(UGameplayStatics::CreateSaveGameObject(UMPSaveGame::StaticClass())))
	{
		Save->BestSplits = BestSplits;
		UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
	}
}
