#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MPCameraEffectsComponent.generated.h"

class UCameraComponent;
class UMPCharacterMovementComponent;
class USpringArmComponent;

UCLASS(ClassGroup = (MP), meta = (BlueprintSpawnableComponent))
class MOVEMENTPLAYGROUND_API UMPCameraEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPCameraEffectsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnMomentumBoost();

	void UpdateFieldOfView(float DeltaTime);

	void UpdateSlideOffset(float DeltaTime);

	[[nodiscard]] float GetSpeedAlpha() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|FOV", meta = (ForceUnits = "deg", ClampMin = "0.0"))
	float SpeedFOVBonus = 15.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|FOV", meta = (ClampMin = "0.0"))
	float SpeedFOVInterpSpeed = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|FOV", meta = (ForceUnits = "deg", ClampMin = "0.0"))
	float BoostFOVPunch = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|FOV", meta = (ClampMin = "0.0"))
	float BoostFOVRecoverySpeed = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|Slide", meta = (ForceUnits = "cm"))
	float SlideHeightOffset = -30.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|Slide", meta = (ForceUnits = "cm"))
	float SlideArmLengthOffset = -60.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Camera|Slide", meta = (ClampMin = "0.0"))
	float SlideInterpSpeed = 8.f;

	UPROPERTY(Transient)
	TObjectPtr<UMPCharacterMovementComponent> Movement;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	float BaseFOV = 90.f;
	float SpeedFOV = 0.f;
	float PunchFOV = 0.f;

	float BaseArmLength = 0.f;
	FVector BaseSocketOffset = FVector::ZeroVector;
	float SlideAlpha = 0.f;
};
