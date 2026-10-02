#include "MP/MPCameraEffectsComponent.h"
#include "MP/MPCharacterMovementComponent.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"

UMPCameraEffectsComponent::UMPCameraEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMPCameraEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Movement || !Camera || !CameraBoom)
	{
		return;
	}

	UpdateFieldOfView(DeltaTime);
	UpdateSlideOffset(DeltaTime);
}

void UMPCameraEffectsComponent::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character)
	{
		return;
	}

	Movement = Cast<UMPCharacterMovementComponent>(Character->GetCharacterMovement());
	Camera = Character->FindComponentByClass<UCameraComponent>();
	CameraBoom = Character->FindComponentByClass<USpringArmComponent>();

	if (Camera)
	{
		BaseFOV = Camera->FieldOfView;
	}

	if (CameraBoom)
	{
		BaseArmLength = CameraBoom->TargetArmLength;
		BaseSocketOffset = CameraBoom->SocketOffset;
	}

	if (Movement)
	{
		AddTickPrerequisiteComponent(Movement);
		Movement->OnMomentumBoost.AddUObject(this, &UMPCameraEffectsComponent::OnMomentumBoost);
	}
}

void UMPCameraEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Movement)
	{
		Movement->OnMomentumBoost.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UMPCameraEffectsComponent::OnMomentumBoost()
{
	PunchFOV = BoostFOVPunch;
}

void UMPCameraEffectsComponent::UpdateFieldOfView(float DeltaTime)
{
	SpeedFOV = FMath::FInterpTo(SpeedFOV, GetSpeedAlpha() * SpeedFOVBonus, DeltaTime, SpeedFOVInterpSpeed);
	PunchFOV = FMath::FInterpTo(PunchFOV, 0.f, DeltaTime, BoostFOVRecoverySpeed);

	Camera->SetFieldOfView(BaseFOV + SpeedFOV + PunchFOV);
}

void UMPCameraEffectsComponent::UpdateSlideOffset(float DeltaTime)
{
	const float TargetAlpha = Movement->IsSliding() ? 1.f : 0.f;

	SlideAlpha = FMath::FInterpTo(SlideAlpha, TargetAlpha, DeltaTime, SlideInterpSpeed);

	CameraBoom->TargetArmLength = BaseArmLength + SlideArmLengthOffset * SlideAlpha;
	CameraBoom->SocketOffset = BaseSocketOffset + FVector(0.f, 0.f, SlideHeightOffset * SlideAlpha);
}

float UMPCameraEffectsComponent::GetSpeedAlpha() const
{
	const float Speed = static_cast<float>(Movement->Velocity.Size());

	return FMath::GetMappedRangeValueClamped(
		FVector2f(Movement->MaxWalkSpeed, Movement->GetMaxMomentumSpeed()), FVector2f(0.f, 1.f), Speed);
}
