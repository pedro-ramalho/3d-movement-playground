#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPMovementTypes.h"

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
