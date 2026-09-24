// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogMPMovement);

void UMPCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UMPCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	
}

void UMPCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	
	UE_LOG(
		LogMPMovement,
		Log,
		TEXT("Mode: %s (%d) -> %s (%d)"),
		*UEnum::GetValueAsString(PreviousMovementMode),
		PreviousCustomMode,
		*UEnum::GetValueAsString(MovementMode),
		CustomMovementMode
	);
}
