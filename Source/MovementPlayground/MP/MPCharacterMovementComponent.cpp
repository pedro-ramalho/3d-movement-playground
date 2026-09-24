// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogMPMovement);

UMPCharacterMovementComponent::UMPCharacterMovementComponent()
{
	// Rotation properties
	bOrientRotationToMovement = true;
	RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	
	// Jump properties
	JumpZVelocity = 500.0f;
	AirControl = 0.35f;
	BrakingDecelerationFalling = 1500.0f;
	
	// Walking properties
	MaxWalkSpeed = 500.f;
	MinAnalogWalkSpeed = 20.f;
	BrakingDecelerationWalking = 2000.f;
}

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
