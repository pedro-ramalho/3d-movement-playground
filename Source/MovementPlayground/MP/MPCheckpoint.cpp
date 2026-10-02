// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCheckpoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

AMPCheckpoint::AMPCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;

	Trigger->SetBoxExtent(FVector(50.f, 400.f, 300.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	SpawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Trigger);
	SpawnPoint->SetRelativeLocation(FVector(-200.f, 0.f, 0.f));
	SpawnPoint->ArrowSize = 2.f;
}

FTransform AMPCheckpoint::GetSpawnTransform() const
{
	return FTransform(SpawnPoint->GetComponentRotation(), SpawnPoint->GetComponentLocation());
}

void AMPCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMPCheckpoint::OnTriggerBeginOverlap);
}

void AMPCheckpoint::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);

	if (!Pawn || !Pawn->IsPlayerControlled() || OtherComp != Pawn->GetRootComponent())
	{
		return;
	}

	OnPlayerReached.Broadcast(this);
}
