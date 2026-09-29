// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogMPMovement);

static TAutoConsoleVariable<int32> CVarMPDebugMovement(
	TEXT("mp.Debug.Movement"),
	0,
	TEXT("Show MP movement debug overlay, 0: off, 1: on"),
	ECVF_Cheat
);

bool UMPCharacterMovementComponent::IsDebugEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarMPDebugMovement.GetValueOnGameThread() != 0;
#else
	return false;
#endif
}

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
	MaxWalkSpeedCrouched = 100.f;
	MinAnalogWalkSpeed = 20.f;
	BrakingDecelerationWalking = 2000.f;
}

void UMPCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	TryEnterGrapple();
	TryEnterSlide();
	TryEnterWallRun();
	
	FHitResult KickHit;
	bHasKickCandidate = MovementMode == MOVE_Falling && FindKickableWall(KickHit);

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UMPCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (!IsDebugEnabled() || !GEngine)
	{
		return;
	}

	constexpr int32 ModeKey = 1;
	constexpr int32 SpeedKey = 2;
	constexpr int32 SlideKey = 3;
	constexpr int32 CrouchKey = 4;
	constexpr int32 WallRunKey = 5;
	constexpr int32 WallKickKey = 6;
	constexpr int32 GrappleKey = 8;

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
	
	GEngine->AddOnScreenDebugMessage(
		WallRunKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Wall candidate: %s"), bHasWallCandidate ? TEXT("yes") : TEXT("no"))
	);

	GEngine->AddOnScreenDebugMessage(
		WallKickKey,
		0.f,
		FColor::Cyan,
		FString::Printf(TEXT("Wall kick: %s"), bHasKickCandidate ? TEXT("yes") : TEXT("no"))
	);
	
	GEngine->AddOnScreenDebugMessage(
		GrappleKey,
		0.f,
		FColor::Cyan,
		IsGrappling()
			? FString::Printf(TEXT("Rope: %.0f / %.0f cm"), RopeLength, TargetRopeLength)
			: FString(TEXT("Rope: -"))
	);
	
	if (IsGrappling())
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), GrappleAnchor, FColor::Green, false, -1.f, 0, 2.f);

#endif
}

void UMPCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (static_cast<EMPCustomMovementMode>(CustomMovementMode))
	{
	case EMPCustomMovementMode::Slide:
		PhysSlide(deltaTime, Iterations);
		break;
		
	case EMPCustomMovementMode::WallRun:
		PhysWallRun(deltaTime, Iterations);
		break;
		
	case EMPCustomMovementMode::Grapple:
		PhysGrapple(deltaTime, Iterations);
		break;
	
	default:
		break;
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
		return IsJumpAllowed() && CanStandUp();
	
	if (IsCustomMovementMode(EMPCustomMovementMode::WallRun))
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

bool UMPCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	bLastJumpWasWallKick = false;

	if (IsCustomMovementMode(EMPCustomMovementMode::WallRun) && CharacterOwner && CharacterOwner->CanJump())
	{
		const FVector AlongWall(Velocity.X, Velocity.Y, 0.f);
		
		Velocity = AlongWall + WallRunNormal * WallJumpOutSpeed + FVector::UpVector * WallJumpUpSpeed;
		SetMovementMode(MOVE_Falling);
		
		return true;
	}
	
	FHitResult KickHit;
	if (FindKickableWall(KickHit))
	{
		const FVector WallNormal = KickHit.ImpactNormal.GetSafeNormal2D();
		const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallNormal);
		
		Velocity = AlongWall + WallNormal * WallKickOutSpeed + FVector::UpVector * WallKickUpSpeed;

		// Face where the kick sends us: the approach direction mirrored off the wall, like Super Mario 64
		const FVector KickDirection = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
		MoveUpdatedComponent(FVector::ZeroVector, KickDirection.ToOrientationQuat(), false);

		LastWall = KickHit.GetComponent();
		bIsWallKickFlight = true;
		bLastJumpWasWallKick = true;

		return true;
	}
	
	return Super::DoJump(bReplayingMoves, DeltaTime);
}

FRotator UMPCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	if (IsWallRunning() && !Velocity.IsNearlyZero())
		return Velocity.GetSafeNormal2D().Rotation();

	// Like Super Mario 64, the facing set by the kick holds for the whole flight
	if (bIsWallKickFlight && IsFalling())
		return CurrentRotation;

	if (IsGrappling())
	{
		const FVector SwingDirection = Velocity.GetSafeNormal2D();
		if (SwingDirection.IsNearlyZero())
			return CurrentRotation;
		
		const FVector CurrentForward = CurrentRotation.Vector().GetSafeNormal2D();
		const FVector Facing = FVector::DotProduct(SwingDirection, CurrentForward) >= 0.f ? SwingDirection : -SwingDirection;
		
		return Facing.Rotation();
	}
	
	return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}

float UMPCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if (bIsWallKickFlight && IsFalling())
		return 0.f;

	return Super::GetMaxBrakingDeceleration();
}

FVector UMPCharacterMovementComponent::GetAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration)
{
	if (bIsWallKickFlight)
	{
		// Input can only stretch or shorten the kick along its own direction, not steer it
		const FVector KickDirection = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
		const FVector AlongKick = KickDirection * FVector::DotProduct(FallAcceleration, KickDirection);

		return Super::GetAirControl(DeltaTime, TickAirControl * WallKickAirControl, AlongKick);
	}

	return Super::GetAirControl(DeltaTime, TickAirControl, FallAcceleration);
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

void UMPCharacterMovementComponent::EvalPreviousCustomMovementMode(const EMPCustomMovementMode Mode)
{
	switch (Mode)
	{
	case EMPCustomMovementMode::Slide: OnExitSlide(); break;
	case EMPCustomMovementMode::WallRun: OnExitWallRun(); break;
	case EMPCustomMovementMode::Grapple: OnExitGrapple(); break;
	default: break;
	}
}

void UMPCharacterMovementComponent::EvalCurrentCustomMovementMode(const EMPCustomMovementMode Mode)
{
	switch (Mode)
	{
	case EMPCustomMovementMode::Slide: OnEnterSlide(); break;
	case EMPCustomMovementMode::WallRun: OnEnterWallRun(); break;
	case EMPCustomMovementMode::Grapple: OnEnterGrapple(); break;
	default: break;
	}
}

void UMPCharacterMovementComponent::LogMovementModeTransition(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) const
{
	UE_LOG(LogMPMovement, Log, TEXT("From %s to %s"),
		*MovementModeToString(PreviousMovementMode, PreviousCustomMode),
		*MovementModeToString(MovementMode, CustomMovementMode));
}

void UMPCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	
	if (PreviousMovementMode == MOVE_Custom)
	{
		EvalPreviousCustomMovementMode(static_cast<EMPCustomMovementMode>(PreviousCustomMode));
	}
	
	if (MovementMode == MOVE_Custom)
	{
		EvalCurrentCustomMovementMode(static_cast<EMPCustomMovementMode>(CustomMovementMode));
	}
	
	if (MovementMode == MOVE_Walking)
	{
		LastWall.Reset();
	}
	
	if (MovementMode != MOVE_Falling)
	{
		bIsWallKickFlight = false;
	}

	LogMovementModeTransition(PreviousMovementMode, PreviousCustomMode);
}

void UMPCharacterMovementComponent::ExitPhysicsTo(const EMovementMode Mode, const float RemainingTime, const int32 Iterations)
{
	SetMovementMode(Mode);
	StartNewPhysics(RemainingTime, Iterations);
}

void UMPCharacterMovementComponent::MoveAndSlide(const FVector& Delta, float TimeTick)
{
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		HandleImpact(Hit, TimeTick, Delta);
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}
}
