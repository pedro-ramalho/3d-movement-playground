// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MPCheckpoint.generated.h"

class AMPCheckpoint;
class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FMPOnPlayerReachedCheckpoint, AMPCheckpoint*);

UCLASS()
class MOVEMENTPLAYGROUND_API AMPCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	AMPCheckpoint();

	[[nodiscard]] FTransform GetSpawnTransform() const;

public:
	FMPOnPlayerReachedCheckpoint OnPlayerReached;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> SpawnPoint;
};
