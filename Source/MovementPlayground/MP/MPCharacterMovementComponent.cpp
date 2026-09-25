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
	NavAgentProps.bCanCrouch = true;
	
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
	// Are we in walking mode?
	if (MovementMode == MOVE_Walking)
	{
		// Do we want to slide?
		if (bWantsToSlide)
		{
			// Do we have enough speed to slide?
			if (Velocity.Size2D() >= SlideEnterSpeed)
			{
				SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::Slide));
			}
		}
	}
	
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
	constexpr int32 CrouchKey = 4;

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
	
	GEngine->AddOnScreenDebugMessage(
		CrouchKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Crouched: %s"), IsCrouching() ? TEXT("yes") : TEXT("no"))
	);
	
#endif
}

void UMPCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (static_cast<EMPCustomMovementMode>(CustomMovementMode))
	{
	case EMPCustomMovementMode::Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	
	default:
		break;
	}
}

void UMPCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	const float Gravity = GetGravityZ();
	
	const FVector GravityVector = FVector(0.f, 0.f, Gravity);
	
	float RemainingTime = deltaTime;
	
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;
		
		if (Velocity.Size() <= SlideExitSpeed || !bWantsToSlide)
		{
			SetMovementMode(MOVE_Walking);
			StartNewPhysics(RemainingTime, Iterations);
			
			return;
		}
		
		FVector Direction = Velocity.GetSafeNormal();
		
		if (CurrentFloor.IsWalkableFloor())
		{
			const FVector SlopeAccelerationVector = FVector::VectorPlaneProject(
				GravityVector, CurrentFloor.HitResult.ImpactNormal
				) * SlideGravityScale;
		
			Velocity += TimeTick * SlopeAccelerationVector;
		
			Direction = FVector::VectorPlaneProject(Velocity.GetSafeNormal(), CurrentFloor.HitResult.ImpactNormal).GetSafeNormal();
		}
	
		const float SpeedValue = Velocity.Size() - (TimeTick * SlideDeceleration);
		const float Speed = FMath::Clamp(SpeedValue, 0.0f, SlideMaxSpeed);
	
		Velocity = Direction * Speed;
	
		const FVector Delta = Velocity * TimeTick;
		const FQuat Rotation = UpdatedComponent->GetComponentQuat();
		FHitResult Hit;
	
		SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);
	
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
	
		if (CurrentFloor.IsWalkableFloor())
		{
			AdjustFloorHeight();
		}
		else
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
			
			return;
		}
	}
}

bool UMPCharacterMovementComponent::IsMovingOnGround() const
{
	if (IsCustomMovementMode(EMPCustomMovementMode::Slide))
		return true;
	
	return Super::IsMovingOnGround();
}

bool UMPCharacterMovementComponent::CanAttemptJump() const
{
	if (IsCustomMovementMode(EMPCustomMovementMode::Slide))
		return IsJumpAllowed();
	
	return Super::CanAttemptJump();
}

bool UMPCharacterMovementComponent::IsCustomMovementMode(EMPCustomMovementMode Mode) const
{
	if (MovementMode != MOVE_Custom)
		return false;
	
	EMPCustomMovementMode CustomMovementType = static_cast<EMPCustomMovementMode>(CustomMovementMode);
	
	return Mode == CustomMovementType;
}

bool UMPCharacterMovementComponent::IsSliding() const
{
	return IsCustomMovementMode(EMPCustomMovementMode::Slide);
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
	
	if (IsCustomMovementMode(EMPCustomMovementMode::Slide))
	{
		bWantsToCrouch = true;
		bCrouchMaintainsBaseLocation = true;
		
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
		AdjustFloorHeight();
	}
	
	if (PreviousMovementMode == MOVE_Custom)
	{
		EMPCustomMovementMode CustomMovementType = static_cast<EMPCustomMovementMode>(PreviousCustomMode);
		
		if (CustomMovementType == EMPCustomMovementMode::Slide)
		{
			bWantsToCrouch = false;
			bWantsToSlide = false;
		}
	}
	
	UE_LOG(LogMPMovement, Log, TEXT("From %s to %s"),
		*MovementModeToString(PreviousMovementMode, PreviousCustomMode),
		*MovementModeToString(MovementMode, CustomMovementMode)
	);
}
