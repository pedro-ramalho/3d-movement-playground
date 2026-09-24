// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MPCharacterMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMPMovement, Log, All);

/**
 * Custom movement modes: slide, wall-run and grapple
 */
UCLASS()
class MOVEMENTPLAYGROUND_API UMPCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	UMPCharacterMovementComponent();
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
};
