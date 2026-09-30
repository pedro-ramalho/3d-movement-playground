#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

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

bool UMPCharacterMovementComponent::CanWallKick() const
{
	return FindKickableWall().IsSet();
}

void UMPCharacterMovementComponent::TryEnterWallRun()
{
	const TOptional<FHitResult> WallHit = FindRunnableWall();
	bHasWallCandidate = WallHit.IsSet();

	if (WallHit)
	{
		WallRunNormal = WallHit->ImpactNormal.GetSafeNormal2D();
		CurrentWall = WallHit->GetComponent();
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::WallRun));
	}
}

void UMPCharacterMovementComponent::OnEnterWallRun()
{
	const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);
	const float OldVelocityZ = Velocity.Z;

	const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallRunNormal).GetSafeNormal();
	Velocity = AlongWall * HVelocity.Size();
	Velocity.Z = FMath::Clamp(OldVelocityZ * WallRunUpSpeedCarry, 0.f, WallRunMaxEntryUpSpeed);

	WallRunStartTime = GetWorld()->GetTimeSeconds();
}

void UMPCharacterMovementComponent::OnExitWallRun()
{
	LastWall = CurrentWall;
}

void UMPCharacterMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	float RemainingTime = deltaTime;
	
	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);

		const TOptional<FHitResult> WallHit = TraceCurrentWall();

		if (!WallHit || ShouldLeaveWallRun())
		{
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
			return;
		}

		RemainingTime -= TimeTick;

		WallRunNormal = WallHit->ImpactNormal.GetSafeNormal2D();
		UpdateWallRunVelocity(TimeTick);

		const FVector Delta = (Velocity - WallRunNormal * WallRunStickSpeed) * TimeTick;
		MoveAndSlide(Delta, TimeTick);

		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

		if (CurrentFloor.IsWalkableFloor() && Velocity.Z <= 0.f)
		{
			ExitPhysicsTo(MOVE_Walking, RemainingTime, Iterations);
			return;
		}
	}
}

float UMPCharacterMovementComponent::GetWallRunTraceLength() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + WallRunTraceDistance;
}

TOptional<FHitResult> UMPCharacterMovementComponent::TraceWall(const FVector& Start, const FVector& End) const
{
	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(TraceWall), false, CharacterOwner);
	FHitResult WallHit;

	if (!GetWorld()->LineTraceSingleByChannel(WallHit, Start, End, ECC_WallRun, CollisionParams))
	{
		return {};
	}

	return WallHit;
}

TOptional<FHitResult> UMPCharacterMovementComponent::TraceCurrentWall() const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();

	return TraceWall(Start, Start - WallRunNormal * GetWallRunTraceLength());
}

bool UMPCharacterMovementComponent::IsTooLowForWallRun() const
{
	const float CapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Start = UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, CapsuleHalfHeight);
	const FVector End = Start - FVector(0.f, 0.f, WallRunMinHeight);

	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(IsTooLowForWallRun), false, CharacterOwner);
	FHitResult FloorHit;

	const bool bFloorHit = GetWorld()->LineTraceSingleByChannel(FloorHit, Start, End,
		UpdatedComponent->GetCollisionObjectType(), CollisionParams);

	DrawDebugTraceLine(Start, bFloorHit ? FloorHit.ImpactPoint : End, bFloorHit ? FColor::Red : FColor::Green);

	return bFloorHit;
}

bool UMPCharacterMovementComponent::ShouldLeaveWallRun() const
{
	const bool bIsTimerExpired = GetWorld()->GetTimeSeconds() - WallRunStartTime >= WallRunMaxDuration;
	const bool bIsTooSlow = Velocity.Size2D() < WallRunMinSpeed;
	const bool bIsSteeringAway = FVector::DotProduct(Acceleration.GetSafeNormal2D(), WallRunNormal) > WallRunSteerAwayThreshold;

	return bIsTimerExpired || bIsTooSlow || bIsSteeringAway;
}

float UMPCharacterMovementComponent::GetWallRunGravityScale() const
{
	const float RunAlpha = FMath::Clamp((GetWorld()->GetTimeSeconds() - WallRunStartTime) / WallRunMaxDuration, 0.f, 1.f);

	return FMath::Lerp(WallRunGravityScaleStart, WallRunGravityScaleEnd, FMath::Pow(RunAlpha, WallRunGravityCurveExponent));
}

void UMPCharacterMovementComponent::UpdateWallRunVelocity(float TimeTick)
{
	const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);
	const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallRunNormal).GetSafeNormal();
	const float NewVelocityZ = Velocity.Z + GetGravityZ() * GetWallRunGravityScale() * TimeTick;

	const float Speed = HVelocity.Size();
	const float NewSpeed = Speed < WallRunMaxSpeed ? FMath::Min(Speed + WallRunAcceleration * TimeTick, WallRunMaxSpeed) : Speed;

	Velocity = AlongWall * NewSpeed;
	Velocity.Z = NewVelocityZ;
}

bool UMPCharacterMovementComponent::IsWallSurface(const FVector& Normal) const
{
	const float MaxNormalZ = FMath::Sin(FMath::DegreesToRadians(WallRunMaxSurfaceTilt));

	return FMath::Abs(Normal.Z) <= MaxNormalZ;
}

TOptional<FHitResult> UMPCharacterMovementComponent::FindRunnableWall() const
{
	if (!HasValidData() || MovementMode != MOVE_Falling)
	{
		return {};
	}

	const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);

	if (HVelocity.IsNearlyZero())
	{
		return {};
	}

	const FVector Side = FVector::CrossProduct(FVector::UpVector, HVelocity.GetSafeNormal()).GetSafeNormal();
	const float TraceLength = GetWallRunTraceLength();

	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector EndA = Start + Side * TraceLength;
	const FVector EndB = Start - Side * TraceLength;

	const TOptional<FHitResult> HitA = TraceWall(Start, EndA);
	const TOptional<FHitResult> HitB = TraceWall(Start, EndB);
	const bool bTooLow = IsTooLowForWallRun();

	const bool bUseA = HitA && (!HitB || HitA->Distance <= HitB->Distance);
	const TOptional<FHitResult>& WallHit = bUseA ? HitA : HitB;

	const bool bRunnable = WallHit
		&& WallHit->GetComponent() != LastWall.Get()
		&& IsWallSurface(WallHit->ImpactNormal)
		&& FVector::VectorPlaneProject(HVelocity, WallHit->ImpactNormal).Size() >= WallRunMinSpeed
		&& !bTooLow;

	DrawDebugTraceLine(Start, HitA ? HitA->ImpactPoint : EndA, GetDebugTraceColor(HitA.IsSet(), bRunnable && bUseA));
	DrawDebugTraceLine(Start, HitB ? HitB->ImpactPoint : EndB, GetDebugTraceColor(HitB.IsSet(), bRunnable && !bUseA));

	if (!bRunnable)
	{
		return {};
	}

	return WallHit;
}

void UMPCharacterMovementComponent::PerformWallJump()
{
	const FVector AlongWall(Velocity.X, Velocity.Y, 0.f);
	const FVector ForwardBoost = AlongWall.GetSafeNormal() * WallJumpForwardBoost;
		
	Velocity = AlongWall + ForwardBoost + WallRunNormal * WallJumpOutSpeed + FVector::UpVector * WallJumpUpSpeed;
	Velocity = Velocity.GetClampedToMaxSize2D(MaxMomentumSpeed);
	SetMovementMode(MOVE_Falling);
}

TOptional<FHitResult> UMPCharacterMovementComponent::FindKickableWall() const
{
	if (!HasValidData() || MovementMode != MOVE_Falling)
	{
		return {};
	}

	const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();

	if (Forward.IsNearlyZero())
	{
		return {};
	}

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
		const float FacingDot = FVector::DotProduct(Forward, -KickHit.ImpactNormal.GetSafeNormal2D());
		const float MinFacingDot = FMath::Cos(FMath::DegreesToRadians(WallKickMaxAngle));

		bKickable = IsWallSurface(KickHit.ImpactNormal)
			&& FacingDot >= MinFacingDot;
	}
	
	const FColor DebugColor = GetDebugTraceColor(bHit, bKickable);
	const FVector SphereCenter = bHit ? KickHit.Location : End;

	DrawDebugTraceLine(Start, SphereCenter, DebugColor);
	DrawDebugTraceSphere(SphereCenter, WallKickTraceRadius, DebugColor);

	if (!bKickable)
	{
		return {};
	}

	return KickHit;
}

void UMPCharacterMovementComponent::PerformWallKick(const FHitResult& KickHit)
{
	const FVector WallNormal = KickHit.ImpactNormal.GetSafeNormal2D();
	const FVector HVelocity(Velocity.X, Velocity.Y, 0.f);
	const FVector AlongWall = FVector::VectorPlaneProject(HVelocity, WallNormal);
		
	Velocity = AlongWall + WallNormal * WallKickOutSpeed + FVector::UpVector * WallKickUpSpeed;

	const FVector KickDirection = FVector(Velocity.X, Velocity.Y, 0.f).GetSafeNormal();
	MoveUpdatedComponent(FVector::ZeroVector, KickDirection.ToOrientationQuat(), false);

	LastWall = KickHit.GetComponent();
	bIsWallKickFlight = true;
	bLastJumpWasWallKick = true;
}
