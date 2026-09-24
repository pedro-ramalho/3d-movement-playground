// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

DEFINE_LOG_CATEGORY(LogMPMovement);

static TAutoConsoleVariable<int32> CVarMPDebugMovement(
	TEXT("mp.Debug.Movement"),
	0,
	TEXT("Show MP movement debug overlay, 0: off, 1: on"),
	ECVF_Cheat
);

UMPCharacterMovementComponent::UMPCharacterMovementComponent()
{
	bWantsToSlide = false;
	
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

void UMPCharacterMovementComponent::SetWantsToSlide(bool bWants)
{
	bWantsToSlide = bWants;
}

void UMPCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (CVarMPDebugMovement.GetValueOnGameThread() == 0 || !GEngine)
	{
		return;
	}

	constexpr int32 ModeKey = 1;
	constexpr int32 SpeedKey = 2;
	constexpr int32 SlideKey = 3;

	GEngine->AddOnScreenDebugMessage(
		ModeKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Current mode: %s"), *MovementModeToString(MovementMode, CustomMovementMode))
	);

	GEngine->AddOnScreenDebugMessage(
		SpeedKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Horizontal speed: %.0f cm/s"), Velocity.Size2D())
	);
	
	GEngine->AddOnScreenDebugMessage(
		SlideKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Wants to slide: %s"), bWantsToSlide ? TEXT("yes") : TEXT("no"))
	);
#endif
}

void UMPCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	
}

FString UMPCharacterMovementComponent::MovementModeToString(EMovementMode Mode, uint8 CustomMode)
{
	if (Mode != MOVE_Custom)
	{
		return UEnum::GetValueAsString(Mode);
	}
	
	EMPCustomMovementMode CustomMovementType = static_cast<EMPCustomMovementMode>(CustomMode);
	
	return UEnum::GetValueAsString(CustomMovementType);
}

void UMPCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	
	UE_LOG(LogMPMovement, Log, TEXT("From %s to %s"),
		*MovementModeToString(PreviousMovementMode, PreviousCustomMode),
		*MovementModeToString(MovementMode, CustomMovementMode)
	);
}
