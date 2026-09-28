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
