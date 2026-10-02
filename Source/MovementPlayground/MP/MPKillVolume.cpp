// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPKillVolume.h"
#include "MovementPlaygroundPlayerController.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

AMPKillVolume::AMPKillVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;

	Trigger->SetBoxExtent(FVector(200.f, 200.f, 50.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->ShapeColor = FColor::Red;
}

void AMPKillVolume::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMPKillVolume::OnTriggerBeginOverlap);
}

void AMPKillVolume::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);

	if (!Pawn || OtherComp != Pawn->GetRootComponent())
	{
		return;
	}

	if (AMovementPlaygroundPlayerController* PlayerController = Pawn->GetController<AMovementPlaygroundPlayerController>())
	{
		GetWorldTimerManager().SetTimerForNextTick(PlayerController, &AMovementPlaygroundPlayerController::RespawnAtCheckpoint);
	}
}
