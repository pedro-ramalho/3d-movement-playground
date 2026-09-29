#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MPGrappleComponent.generated.h"

class ACharacter;
class AMPGrapplePoint;
class UCableComponent;
class UCameraComponent;
class UMPCharacterMovementComponent;

UCLASS(ClassGroup = (MP), meta = (BlueprintSpawnableComponent))
class MOVEMENTPLAYGROUND_API UMPGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMPGrappleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void StartGrapple();

	void StopGrapple();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void OnMovementModeChanged(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void UpdateGrappleTarget();

	AMPGrapplePoint* FindBestGrapplePoint() const;

	void StartGrappleShot();

	void UpdateGrappleShot();

	void HideGrappleCable();

private:
	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "cm"))
	float GrappleMaxRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "deg"))
	float GrappleAimAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "cm"))
	float GrappleMinHeightAbove = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float GrappleShotDuration = 0.12f;

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UMPCharacterMovementComponent> Movement;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UCableComponent> GrappleCable;

	TWeakObjectPtr<AMPGrapplePoint> GrappleTarget;

	float GrappleShotStartTime = -1.f;
};
