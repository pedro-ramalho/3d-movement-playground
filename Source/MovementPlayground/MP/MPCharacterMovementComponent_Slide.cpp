#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

void UMPCharacterMovementComponent::SetWantsToSlide(bool bWants)
{
	bWantsToSlide = bWants;
}

bool UMPCharacterMovementComponent::IsSliding() const
{
	return IsCustomMovementMode(EMPCustomMovementMode::Slide);
}

bool UMPCharacterMovementComponent::CanStandUp() const
{
	if (!HasValidData())
	{
		return false;
	}

	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	const float HalfHeightAdjust = DefaultCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() - Capsule->GetUnscaledCapsuleHalfHeight();
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * Capsule->GetShapeScale();

	constexpr float SweepInflation = UE_KINDA_SMALL_NUMBER * 10.f;
	const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation - ScaledHalfHeightAdjust);

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

void UMPCharacterMovementComponent::TryEnterSlide()
{
	if (MovementMode == MOVE_Walking && bWantsToSlide && Velocity.Size2D() >= SlideEnterSpeed)
	{
		SetMovementMode(MOVE_Custom, static_cast<uint8>(EMPCustomMovementMode::Slide));
	}
}

void UMPCharacterMovementComponent::OnEnterSlide()
{
	bWantsToCrouch = true;
	bCrouchMaintainsBaseLocation = true;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
	AdjustFloorHeight();

	ApplySlideBoost();
}

void UMPCharacterMovementComponent::OnExitSlide()
{
	bWantsToCrouch = false;
	bWantsToSlide = false;
}

void UMPCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	float RemainingTime = deltaTime;

	while (RemainingTime >= MIN_TICK_TIME && Iterations < MaxSimulationIterations)
	{
		Iterations++;
		const float TimeTick = GetSimulationTimeStep(RemainingTime, Iterations);

		if (ShouldLeaveSlide())
		{
			ExitPhysicsTo(MOVE_Walking, RemainingTime, Iterations);
			return;
		}

		RemainingTime -= TimeTick;

		UpdateSlideVelocity(TimeTick);
		MoveAndSlide(Velocity * TimeTick, TimeTick);

		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

		if (!CurrentFloor.IsWalkableFloor())
		{
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
			return;
		}

		AdjustFloorHeight();
	}
}

void UMPCharacterMovementComponent::ApplySlideBoost()
{
	const float Now = GetWorld()->GetTimeSeconds();

	if (Now < SlideBoostReadyTime)
	{
		return;
	}

	const float Speed = Velocity.Size2D();
	const float BoostedSpeed = FMath::Max(Speed, FMath::Min(Speed + SlideBoost, SlideMaxSpeed));

	Velocity = Velocity.GetSafeNormal2D() * BoostedSpeed + FVector(0.f, 0.f, Velocity.Z);
	SlideBoostReadyTime = Now + SlideBoostCooldown;

	if (BoostedSpeed > Speed)
	{
		OnMomentumBoost.Broadcast();
	}
}

bool UMPCharacterMovementComponent::ShouldLeaveSlide() const
{
	const bool bTooSlow = Velocity.Size() < SlideExitSpeed;
	const bool bReleased = !bWantsToSlide && CanStandUp();

	return bTooSlow || bReleased;
}

void UMPCharacterMovementComponent::UpdateSlideVelocity(float TimeTick)
{
	const FVector FloorNormal = CurrentFloor.IsWalkableFloor() ? CurrentFloor.HitResult.ImpactNormal : FVector::UpVector;

	float Speed = Velocity.Size();

	const FVector InputDirection = FVector::VectorPlaneProject(Acceleration.GetSafeNormal(), FloorNormal);
	const FVector SteerInput = FVector::VectorPlaneProject(InputDirection, Velocity.GetSafeNormal());
	Velocity += SteerInput * SlideSteering * TimeTick;
	Velocity = Velocity.GetSafeNormal() * Speed;

	Velocity += FVector::VectorPlaneProject(FVector(0.f, 0.f, GetGravityZ()), FloorNormal) * SlideGravityScale * TimeTick;

	Speed = FMath::Clamp(Velocity.Size() - SlideFriction * TimeTick, 0.f, SlideMaxSpeed);
	Velocity = FVector::VectorPlaneProject(Velocity, FloorNormal).GetSafeNormal() * Speed;
}
