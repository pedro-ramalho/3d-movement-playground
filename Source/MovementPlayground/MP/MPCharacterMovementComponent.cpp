// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
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
	const bool bCooldownReady = GetWorld()->GetTimeSeconds() - GrappleEndTime >= GrappleCooldown;
	if (bWantsToGrapple && !IsGrappling() && bCooldownReady && GrappleAnchorActor.IsValid())
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::Grapple));
	
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
	
	FHitResult WallHit;
	bHasWallCandidate = MovementMode == MOVE_Falling && FindRunnableWall(WallHit);
	if (bHasWallCandidate)
	{
		WallRunNormal = WallHit.ImpactNormal.GetSafeNormal2D();
		CurrentWall = WallHit.GetComponent();
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::WallRun));
	}

	FHitResult KickHit;
	bHasKickCandidate = MovementMode == MOVE_Falling && FindKickableWall(KickHit);

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UMPCharacterMovementComponent::SetWantsToSlide(bool bWants)
{
	bWantsToSlide = bWants;
}

bool UMPCharacterMovementComponent::CanStandUp() const
{
	if (!HasValidData())
	{
		return false;
	}

	// Same test as UCharacterMovementComponent::UnCrouch, for the bCrouchMaintainsBaseLocation case
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - Capsule->GetUnscaledCapsuleHalfHeight();
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * Capsule->GetShapeScale();

	// Slightly taller than standing, so a ceiling at exactly standing height still blocks
	const float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
	const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust);

	// The feet stay put, so the standing capsule's center sits higher than the current one
	const FVector StandingLocation = UpdatedComponent->GetComponentLocation()
		+ (StandingCapsuleShape.GetCapsuleHalfHeight() - Capsule->GetScaledCapsuleHalfHeight()) * -GetGravityDirection();

	FCollisionQueryParams CapsuleParams(SCENE_QUERY_STAT(MPStandUpTest), false, CharacterOwner);
	FCollisionResponseParams ResponseParams;
	InitCollisionParams(CapsuleParams, ResponseParams);

	const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(
		StandingLocation,
		GetWorldToGravityTransform(),
		UpdatedComponent->GetCollisionObjectType(),
		StandingCapsuleShape,
		CapsuleParams,
		ResponseParams
	);

	return !bBlocked;
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
			? FString::Printf(TEXT("Rope: %.0f cm"), RopeLength)
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

void UMPCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	float RemainingTime = deltaTime;

	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		RemainingTime -= TimeTick;

		// The slide lasts exactly as long as the slide montage; its root motion sets Velocity before we get here
		if (!CharacterOwner || !CharacterOwner->IsPlayingRootMotion())
		{
			SetMovementMode(MOVE_Walking);
			StartNewPhysics(RemainingTime, Iterations);

			return;
		}

		// Keep the animation's speed, but make it follow the floor so slides go up and down ramps
		FVector Direction = Velocity.GetSafeNormal();

		if (CurrentFloor.IsWalkableFloor())
		{
			Direction = FVector::VectorPlaneProject(Direction, CurrentFloor.HitResult.ImpactNormal).GetSafeNormal();
		}

		Velocity = Direction * Velocity.Size();
	
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

void UMPCharacterMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	float RemainingTime = deltaTime;
	
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		
		const float TraceLength = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + WallRunTraceDistance;
		const FVector Start = UpdatedComponent->GetComponentLocation();
		const FVector End = Start - WallRunNormal * TraceLength;
		
		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(PhysWallRun), false, CharacterOwner);
		FHitResult WallHit;
		
		if (!GetWorld()->LineTraceSingleByChannel(WallHit, Start, End, ECC_WallRun, CollisionParams))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
			
			return;
		}
		
		const bool bIsTimerExpired = GetWorld()->GetTimeSeconds() - WallRunStartTime >= WallRunMaxDuration;
		const bool bIsTooSlow = Velocity.Size2D() < WallRunMinSpeed;
		const bool bIsSteeringAway = FVector::DotProduct(Acceleration.GetSafeNormal2D(), WallRunNormal) > WallRunSteerAwayThreshold;

		if (bIsTimerExpired || bIsTooSlow || bIsSteeringAway)
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
			
			return;
		}
		
		RemainingTime -= TimeTick;
		
		WallRunNormal = WallHit.ImpactNormal.GetSafeNormal2D();
		
		const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);
		const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallRunNormal).GetSafeNormal();
		const float RunAlpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - WallRunStartTime) / WallRunMaxDuration, 0.f, 1.f);
		const float WallRunGravityScale = FMath::Lerp(WallRunGravityScaleStart, WallRunGravityScaleEnd, FMath::Pow(RunAlpha, WallRunGravityCurveExponent));
		const float NewVelocityZ = Velocity.Z + GetGravityZ() * WallRunGravityScale * TimeTick;
		
		Velocity = AlongWall * HVelocity.Size();
		Velocity.Z = NewVelocityZ;
		
		const FVector Delta = (Velocity - WallRunNormal * WallRunStickSpeed) * TimeTick;
		
		FHitResult MoveHit;
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, MoveHit);
		
		if (MoveHit.IsValidBlockingHit())
		{
			SlideAlongSurface(Delta, 1.f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
		}

		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

		if (CurrentFloor.IsWalkableFloor() && Velocity.Z <= 0.f)
		{
			SetMovementMode(MOVE_Walking);
			StartNewPhysics(RemainingTime, Iterations);

			return;
		}
	}
}

void UMPCharacterMovementComponent::PhysGrapple(float deltaTime, int32 Iterations)
{
	float RemainingTime = deltaTime;
	
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);
		
		if (ShouldReleaseGrapple())
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(RemainingTime, Iterations);
		
			return;
		}
		
		RemainingTime -= TimeTick;
		
		Velocity.Z += GetGravityZ() * GrappleGravityScale * TimeTick;
		
		ApplyRopeToVelocity();
		
		const FVector Delta = Velocity * TimeTick;
		
		FHitResult Hit;
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, TimeTick, Delta);
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}
		
		ApplyRopeToPosition();
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

bool UMPCharacterMovementComponent::IsSliding() const
{
	return IsCustomMovementMode(EMPCustomMovementMode::Slide);
}

bool UMPCharacterMovementComponent::IsWallRunning() const
{
	return IsCustomMovementMode(EMPCustomMovementMode::WallRun);
}

float UMPCharacterMovementComponent::GetWallRunSide() const
{
	if (!IsWallRunning() || !CharacterOwner)
		return 0.f;
	
	const float Product = FVector::DotProduct(WallRunNormal, CharacterOwner->GetActorRightVector());
	
	return Product > 0.f ? -1.f : 1.f;
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
	
	if (IsCustomMovementMode(EMPCustomMovementMode::WallRun))
	{
		FVector HVelocity = Velocity;
		HVelocity.Z = 0.f;

		const float OldVelocityZ = Velocity.Z;

		const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallRunNormal).GetSafeNormal();
		Velocity = AlongWall * HVelocity.Size();
		Velocity.Z = FMath::Clamp(OldVelocityZ * WallRunUpSpeedCarry, 0.f, WallRunMaxEntryUpSpeed);

		WallRunStartTime = GetWorld()->GetTimeSeconds();
	}
	
	if (IsGrappling())
	{
		RopeLength = FVector::Dist(UpdatedComponent->GetComponentLocation(), GrappleAnchor);
		GrappleStartTime = GetWorld()->GetTimeSeconds();
	}
	
	if (PreviousMovementMode == MOVE_Custom)
	{
		EMPCustomMovementMode CustomMovementType = static_cast<EMPCustomMovementMode>(PreviousCustomMode);
		
		if (CustomMovementType == EMPCustomMovementMode::Slide)
		{
			bWantsToCrouch = false;
			bWantsToSlide = false;
		}
		
		if (CustomMovementType == EMPCustomMovementMode::WallRun)
			LastWall = CurrentWall;
			
		if (CustomMovementType == EMPCustomMovementMode::Grapple)
		{
			GrappleEndTime = GetWorld()->GetTimeSeconds();
			bWantsToGrapple = false;
		}
	}
	
	if (MovementMode == MOVE_Walking)
		LastWall.Reset();

	if (MovementMode != MOVE_Falling)
		bIsWallKickFlight = false;
	
	UE_LOG(LogMPMovement, Log, TEXT("From %s to %s"),
		*MovementModeToString(PreviousMovementMode, PreviousCustomMode),
		*MovementModeToString(MovementMode, CustomMovementMode)
	);
}

bool UMPCharacterMovementComponent::FindRunnableWall(FHitResult& OutWallHit) const
{
	if (!HasValidData())
		return false;

	FVector HVelocity = Velocity;
	HVelocity.Z = 0.f;

	if (HVelocity.IsNearlyZero())
		return false;

	const FVector HDirection = HVelocity.GetSafeNormal();
	const FVector PDirection = FVector::CrossProduct(FVector::UpVector, HDirection).GetSafeNormal();

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	const float TraceLength = CapsuleRadius + WallRunTraceDistance;

	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(FindRunnableWall), false, CharacterOwner);

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector EndA = Start + PDirection * TraceLength;
	const FVector EndB = Start - PDirection * TraceLength;
	
	FHitResult HitA, HitB;
	
	const bool bHitA = GetWorld()->LineTraceSingleByChannel(HitA, Start, EndA, ECC_WallRun, CollisionParams);
	const bool bHitB = GetWorld()->LineTraceSingleByChannel(HitB, Start, EndB, ECC_WallRun, CollisionParams);

	const FVector DownStart = Start - FVector(0.f, 0.f, CapsuleHalfHeight);
	const FVector DownEnd = DownStart - FVector(0.f, 0.f, WallRunMinHeight);

	FHitResult FloorHit;
	const bool bFloorHit = GetWorld()->LineTraceSingleByChannel(FloorHit, DownStart, DownEnd,
		UpdatedComponent->GetCollisionObjectType(), CollisionParams);

	const bool bUseA = bHitA && (!bHitB || HitA.Distance <= HitB.Distance);
	const FHitResult& WallHit = bUseA ? HitA : HitB;

	bool bRunnable = (bHitA || bHitB) && WallHit.GetComponent() != LastWall.Get();

	if (bRunnable)
	{
		const float MaxNormalZ = FMath::Sin(FMath::DegreesToRadians(WallRunMaxSurfaceTilt));
		const FVector AlongWallVelocity = FVector::VectorPlaneProject(HVelocity, WallHit.ImpactNormal);

		bRunnable = FMath::Abs(WallHit.ImpactNormal.Z) <= MaxNormalZ
			&& AlongWallVelocity.Size() >= WallRunMinSpeed
			&& !bFloorHit;
	}

#if !UE_BUILD_SHIPPING
	if (CVarMPDebugMovement.GetValueOnGameThread() != 0)
	{
		auto SideColor = [bRunnable](bool bHit, bool bChosen)
		{
			if (!bHit)
				return FColor::Silver;

			return (bRunnable && bChosen) ? FColor::Green : FColor::Red;
		};

		DrawDebugLine(GetWorld(), Start, bHitA ? HitA.ImpactPoint : EndA, SideColor(bHitA, bUseA), false, -1.f, 0, 1.5f);
		DrawDebugLine(GetWorld(), Start, bHitB ? HitB.ImpactPoint : EndB, SideColor(bHitB, !bUseA), false, -1.f, 0, 1.5f);
		DrawDebugLine(GetWorld(), DownStart, bFloorHit ? FloorHit.ImpactPoint : DownEnd,
			bFloorHit ? FColor::Red : FColor::Green, false, -1.f, 0, 1.5f);
	}
#endif

	if (bRunnable)
	{
		OutWallHit = WallHit;
	}

	return bRunnable;
}

bool UMPCharacterMovementComponent::FindKickableWall(FHitResult& OutKickHit) const
{
	if (!HasValidData() || MovementMode != MOVE_Falling)
		return false;
	
	const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
	
	if (Forward.IsNearlyZero())
		return false;
	
	const float CapsuleRadius = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float TraceLength = CapsuleRadius + WallKickTraceDistance;
	
	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(FindKickableWall), false, CharacterOwner);
	
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + Forward * TraceLength;
	
	FHitResult KickHit;
	const bool bHit = GetWorld()->SweepSingleByChannel(KickHit, Start, End, FQuat::Identity, ECC_WallRun,
		FCollisionShape::MakeSphere(WallKickTraceRadius), CollisionParams);
	
	bool bKickable = bHit && KickHit.GetComponent() != LastWall.Get();
	
	if (bKickable)
	{
		const float MaxNormalZ = FMath::Sin(FMath::DegreesToRadians(WallRunMaxSurfaceTilt));
		const float FacingDot = FVector::DotProduct(Forward, -KickHit.ImpactNormal.GetSafeNormal2D());
		const float MinFacingDot = FMath::Cos(FMath::DegreesToRadians(WallKickMaxAngle));
		
		bKickable = FMath::Abs(KickHit.ImpactNormal.Z) <= MaxNormalZ
			&& FacingDot >= MinFacingDot;
	}
	
#if !UE_BUILD_SHIPPING
	if (CVarMPDebugMovement.GetValueOnGameThread() != 0)
	{
		const FColor Color = !bHit ? FColor::Silver : (bKickable ? FColor::Green : FColor::Red);
		const FVector SphereCenter = bHit ? KickHit.Location : End;
		
		DrawDebugLine(GetWorld(), Start, SphereCenter, Color, false, -1.f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), SphereCenter, WallKickTraceRadius, 12, Color, false, -1.f, 0, 1.f);
	}
#endif
	
	if (bKickable)
	{
		OutKickHit = KickHit;
	}
	
	return bKickable;
}

bool UMPCharacterMovementComponent::CanWallKick() const
{
	FHitResult Hit;
	
	return FindKickableWall(Hit);
}

void UMPCharacterMovementComponent::RequestGrapple(const FVector& Anchor, const AActor* AnchorActor)
{
	GrappleAnchor = Anchor;
	GrappleAnchorActor = AnchorActor;
	bWantsToGrapple = true;
}

void UMPCharacterMovementComponent::ReleaseGrapple()
{
	bWantsToGrapple = false;
}

bool UMPCharacterMovementComponent::IsGrappling() const
{
	return IsCustomMovementMode(EMPCustomMovementMode::Grapple);
}

bool UMPCharacterMovementComponent::ShouldReleaseGrapple() const
{
	const bool bReleased = !bWantsToGrapple;
	const bool bAnchorGone = !GrappleAnchorActor.IsValid();

	return bReleased || bAnchorGone;
}

void UMPCharacterMovementComponent::ApplyRopeToVelocity()
{
	const FVector ToCharacter = UpdatedComponent->GetComponentLocation() - GrappleAnchor;
	const float Distance = ToCharacter.Size();
	
	if (Distance < RopeLength - 1.f || Distance < UE_KINDA_SMALL_NUMBER)
		return;
	
	const FVector Outward = ToCharacter / Distance;
	const float OutwardSpeed = FVector::DotProduct(Velocity, Outward);
	
	if (OutwardSpeed > 0.f)
		Velocity -= Outward * OutwardSpeed;
}

void UMPCharacterMovementComponent::ApplyRopeToPosition()
{
	const FVector Location = UpdatedComponent->GetComponentLocation();
	const FVector ToCharacter = Location - GrappleAnchor;
	
	const float Distance = ToCharacter.Size();
	
	if (Distance <= RopeLength || Distance < UE_KINDA_SMALL_NUMBER)
		return;
	
	const FVector Target = GrappleAnchor + ToCharacter / Distance * RopeLength;
	
	FHitResult Hit;
	SafeMoveUpdatedComponent(Target - Location, UpdatedComponent->GetComponentQuat(), true, Hit);
	
	ApplyRopeToVelocity();
}
