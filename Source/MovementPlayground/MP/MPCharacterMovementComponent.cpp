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

#if !UE_BUILD_SHIPPING
namespace
{
	const TCHAR* YesNo(bool bValue)
	{
		return bValue ? TEXT("yes") : TEXT("no");
	}
}
#endif

UMPCharacterMovementComponent::UMPCharacterMovementComponent()
{
	NavAgentProps.bCanCrouch = true;

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

	bHasKickCandidate = FindKickableWall().IsSet();

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UMPCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (!IsDebugEnabled())
	{
		return;
	}

	PrintDebugMessage(EMPDebugKey::Mode, FString::Printf(TEXT("Current mode: %s"), *MovementModeToString(MovementMode, CustomMovementMode)));
	PrintDebugMessage(EMPDebugKey::Speed, FString::Printf(TEXT("Horizontal speed: %.0f cm/s"), Velocity.Size2D()));
	PrintDebugMessage(EMPDebugKey::Slide, FString::Printf(TEXT("Wants to slide: %s"), YesNo(bWantsToSlide)));
	PrintDebugMessage(EMPDebugKey::Crouch, FString::Printf(TEXT("Crouched: %s"), YesNo(IsCrouching())));
	PrintDebugMessage(EMPDebugKey::WallRun, FString::Printf(TEXT("Wall candidate: %s"), YesNo(bHasWallCandidate)));
	PrintDebugMessage(EMPDebugKey::WallKick, FString::Printf(TEXT("Wall kick: %s"), YesNo(bHasKickCandidate)));

	if (IsGrappling())
	{
		PrintDebugMessage(EMPDebugKey::Rope, FString::Printf(TEXT("Rope: %.0f / %.0f cm"), RopeLength, TargetRopeLength));
		DrawDebugLine(GetWorld(), UpdatedComponent->GetComponentLocation(), GrappleAnchor, FColor::Green, false, -1.f, 0, 2.f);
	}
	else
	{
		PrintDebugMessage(EMPDebugKey::Rope, TEXT("Rope: -"));
	}
#endif
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

bool UMPCharacterMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	bLastJumpWasWallKick = false;
	
	if (IsCustomMovementMode(EMPCustomMovementMode::WallRun) && CharacterOwner && CharacterOwner->CanJump())
	{
		PerformWallJump();
		CoyoteTimeEnd = -1.f;
		return true;
	}
	
	if (const TOptional<FHitResult> KickHit = FindKickableWall())
	{
		PerformWallKick(*KickHit);
		CoyoteTimeEnd = -1.f;
		return true;
	}
	
	const bool bJumped = Super::DoJump(bReplayingMoves, DeltaTime);

	if (bJumped)
	{
		CoyoteTimeEnd = -1.f;
	}

	return bJumped;
}

FRotator UMPCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const
{
	if (IsWallRunning() && !Velocity.IsNearlyZero())
		return Velocity.GetSafeNormal2D().Rotation();

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

	if (IsFalling() && Velocity.Size2D() > MaxWalkSpeed)
		return AirOverspeedBraking;

	return Super::GetMaxBrakingDeceleration();
}

FVector UMPCharacterMovementComponent::GetAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration)
{
	if (bIsWallKickFlight)
	{
		const FVector KickDirection = UpdatedComponent->GetForwardVector().GetSafeNormal2D();
		const FVector AlongKick = KickDirection * FVector::DotProduct(FallAcceleration, KickDirection);

		return Super::GetAirControl(DeltaTime, TickAirControl * WallKickAirControl, AlongKick);
	}

	return Super::GetAirControl(DeltaTime, TickAirControl, FallAcceleration);
}

void UMPCharacterMovementComponent::ResetMovementState()
{
	bWantsToSlide = false;
	bWantsToGrapple = false;
	GrappleAnchorActor.Reset();

	SetMovementMode(MOVE_Falling);

	Velocity = FVector::ZeroVector;
	ClearAccumulatedForces();

	bWantsToCrouch = false;
	UnCrouch();

	bHasWallCandidate = false;
	CurrentWall.Reset();
	LastWall.Reset();

	bHasKickCandidate = false;
	bLastJumpWasWallKick = false;
	bIsWallKickFlight = false;
	CoyoteTimeEnd = -1.f;
}

bool UMPCharacterMovementComponent::IsWithinCoyoteTime() const
{
	return IsFalling() && GetWorld()->GetTimeSeconds() <= CoyoteTimeEnd;
}

bool UMPCharacterMovementComponent::IsCustomMovementMode(EMPCustomMovementMode Mode) const
{
	if (MovementMode != MOVE_Custom)
		return false;
	
	EMPCustomMovementMode CustomMovementType = static_cast<EMPCustomMovementMode>(CustomMovementMode);
	
	return Mode == CustomMovementType;
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

bool UMPCharacterMovementComponent::IsDebugEnabled()
{
#if !UE_BUILD_SHIPPING
	return CVarMPDebugMovement.GetValueOnGameThread() != 0;
#else
	return false;
#endif
}

void UMPCharacterMovementComponent::PrintDebugMessage(EMPDebugKey Key, const FString& Message)
{
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(static_cast<int32>(Key), 0.f, FColor::Cyan, Message);
	}
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

	const bool bLeftGround = PreviousMovementMode == MOVE_Walking
		|| (PreviousMovementMode == MOVE_Custom && static_cast<EMPCustomMovementMode>(PreviousCustomMode) == EMPCustomMovementMode::Slide);

	if (MovementMode == MOVE_Falling && bLeftGround)
	{
		CoyoteTimeEnd = GetWorld()->GetTimeSeconds() + CoyoteTime;
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

FColor UMPCharacterMovementComponent::GetDebugTraceColor(bool bHit, bool bAccepted)
{
	if (!bHit)
	{
		return FColor::Silver;
	}

	return bAccepted ? FColor::Green : FColor::Red;
}

void UMPCharacterMovementComponent::DrawDebugTraceLine(const FVector& Start, const FVector& End, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
	if (IsDebugEnabled())
	{
		DrawDebugLine(GetWorld(), Start, End, Color, false, -1.f, 0, 1.5f);
	}
#endif
}

void UMPCharacterMovementComponent::DrawDebugTraceSphere(const FVector& Center, float Radius, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
	if (IsDebugEnabled())
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 12, Color, false, -1.f, 0, 1.f);
	}
#endif
}
