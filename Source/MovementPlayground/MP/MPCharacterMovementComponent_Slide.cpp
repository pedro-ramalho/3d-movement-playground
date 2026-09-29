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
		RemainingTime -= TimeTick;

		if (!CharacterOwner || !CharacterOwner->IsPlayingRootMotion())
		{
			ExitPhysicsTo(MOVE_Walking, RemainingTime, Iterations);
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
			ExitPhysicsTo(MOVE_Falling, RemainingTime, Iterations);
			return;
		}
	}
}
