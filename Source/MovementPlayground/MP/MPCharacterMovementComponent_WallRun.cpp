#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

void UMPCharacterMovementComponent::TryEnterWallRun()
{
	FHitResult WallHit;
	bHasWallCandidate = MovementMode == MOVE_Falling && FindRunnableWall(WallHit);

	if (bHasWallCandidate)
	{
		WallRunNormal = WallHit.ImpactNormal.GetSafeNormal2D();
		CurrentWall = WallHit.GetComponent();
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
		
		const float TraceLength = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius() + WallRunTraceDistance;
		const FVector Start = UpdatedComponent->GetComponentLocation();
		const FVector End = Start - WallRunNormal * TraceLength;
		
		FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(PhysWallRun), false, CharacterOwner);
		FHitResult WallHit;
		
		if (!GetWorld()->LineTraceSingleByChannel(WallHit, Start, End, ECC_WallRun, CollisionParams))
		{
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
			return;
		}
		
		const bool bIsTimerExpired = GetWorld()->GetTimeSeconds() - WallRunStartTime >= WallRunMaxDuration;
		const bool bIsTooSlow = Velocity.Size2D() < WallRunMinSpeed;
		const bool bIsSteeringAway = FVector::DotProduct(Acceleration.GetSafeNormal2D(), WallRunNormal) > WallRunSteerAwayThreshold;

		if (bIsTimerExpired || bIsTooSlow || bIsSteeringAway)
		{
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
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
		MoveAndSlide(Delta, TimeTick);

		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

		if (CurrentFloor.IsWalkableFloor() && Velocity.Z <= 0.f)
		{
			ExitPhysicsTo(MOVE_Walking, RemainingTime, Iterations);
			return;
		}
	}
}

bool UMPCharacterMovementComponent::IsWallSurface(const FVector& Normal) const
{
	const float MaxNormalZ = FMath::Sin(FMath::DegreesToRadians(WallRunMaxSurfaceTilt));

	return FMath::Abs(Normal.Z) <= MaxNormalZ;
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
		const FVector AlongWallVelocity = FVector::VectorPlaneProject(HVelocity, WallHit.ImpactNormal);

		bRunnable = IsWallSurface(WallHit.ImpactNormal)
			&& AlongWallVelocity.Size() >= WallRunMinSpeed
			&& !bFloorHit;
	}

#if !UE_BUILD_SHIPPING
	if (IsDebugEnabled())
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
		const float FacingDot = FVector::DotProduct(Forward, -KickHit.ImpactNormal.GetSafeNormal2D());
		const float MinFacingDot = FMath::Cos(FMath::DegreesToRadians(WallKickMaxAngle));

		bKickable = IsWallSurface(KickHit.ImpactNormal)
			&& FacingDot >= MinFacingDot;
	}
	
#if !UE_BUILD_SHIPPING
	if (IsDebugEnabled())
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
