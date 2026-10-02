#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

#include "GameFramework/Character.h"

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

float UMPCharacterMovementComponent::GetGrappleSwingAngle() const
{
	if (!IsGrappling() || !CharacterOwner)
		return 0.f;
	
	const FVector Rope = GrappleAnchor - UpdatedComponent->GetComponentLocation();
	const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
	
	return FMath::RadiansToDegrees(FMath::Atan2(-FVector::DotProduct(Rope, Forward), Rope.Z));
}

void UMPCharacterMovementComponent::TryEnterGrapple()
{
	const bool bCooldownReady = GetWorld()->GetTimeSeconds() - GrappleEndTime >= GrappleCooldown;

	if (bWantsToGrapple && !IsGrappling() && bCooldownReady && GrappleAnchorActor.IsValid())
	{
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::Grapple));
	}
}

void UMPCharacterMovementComponent::OnEnterGrapple()
{
	RopeLength = FVector::Dist(UpdatedComponent->GetComponentLocation(), GrappleAnchor);
	TargetRopeLength = FMath::Min(RopeLength, 
		FMath::Max(GrappleMinRopeLength,
		RopeLength * (1.f - GrappleReelFraction))
	);
		
	GrappleStartTime = GetWorld()->GetTimeSeconds();
}

void UMPCharacterMovementComponent::OnExitGrapple()
{
	GrappleEndTime = GetWorld()->GetTimeSeconds();
	bWantsToGrapple = false;
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
			ApplyGrappleReleaseBoost();
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
			return;
		}
		
		RemainingTime -= TimeTick;
		UpdateGrappleReel(TimeTick);

		Velocity.Z += GetGravityZ() * GrappleGravityScale * TimeTick;
		ApplyGrappleSwingInput(TimeTick);
		ApplyRopeToVelocity();
		
		MoveAndSlide(Velocity * TimeTick, TimeTick);
		
		ApplyRopeToPosition();
		
		const bool bPastGrace = GetWorld()->GetTimeSeconds() - GrappleStartTime >= GrappleFloorGraceTime;
		if (bPastGrace && Velocity.Z < 0.f)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

			if (CurrentFloor.IsWalkableFloor() && CurrentFloor.FloorDist <= MAX_FLOOR_DIST)
			{
				ExitPhysicsTo(MOVE_Walking, RemainingTime, Iterations);
				return;
			}
		}
	}
}

bool UMPCharacterMovementComponent::ShouldReleaseGrapple() const
{
	const bool bReleased = !bWantsToGrapple;
	const bool bAnchorGone = !GrappleAnchorActor.IsValid();

	return bReleased || bAnchorGone;
}

void UMPCharacterMovementComponent::ApplyGrappleReleaseBoost()
{
	if (bWantsToGrapple || Velocity.Z < 0.f || Velocity.Size() < GrappleReleaseMinSpeed)
	{
		return;
	}

	Velocity += Velocity.GetSafeNormal() * GrappleReleaseBoost;
	Velocity = Velocity.GetClampedToMaxSize2D(MaxMomentumSpeed);
}

void UMPCharacterMovementComponent::UpdateGrappleReel(float TimeTick)
{
	RopeLength = FMath::FInterpConstantTo(RopeLength, TargetRopeLength, TimeTick, GrappleReelSpeed);
}

void UMPCharacterMovementComponent::ApplyGrappleSwingInput(float TimeTick)
{
	const FVector RopeDirection = (GrappleAnchor - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
	const FVector Input = Acceleration / FMath::Max(GetMaxAcceleration(), 1.f);
	const FVector SwingInput = FVector::VectorPlaneProject(Input, RopeDirection);

	Velocity += SwingInput * GrappleSwingControl * TimeTick;
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
