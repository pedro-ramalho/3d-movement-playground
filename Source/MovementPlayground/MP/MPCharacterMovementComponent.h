// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MPCharacterMovementComponent.generated.h"

enum class EMPCustomMovementMode : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogMPMovement, Log, All);

/**
 * Custom movement modes: slide, wall-run and grapple
 */
UCLASS()
class MOVEMENTPLAYGROUND_API UMPCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
	bool bWantsToSlide;
	
	bool bHasWallCandidate = false;

	bool bHasKickCandidate = false;
	
	bool bLastJumpWasWallKick = false;
	
	bool bWantsToGrapple = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Slide", meta = (ForceUnits = "cm/s"))
	float SlideEnterSpeed = 350.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s"))
	float WallRunMinSpeed = 300.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm"))
	float WallRunMinHeight = 60.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm"))
	float WallRunTraceDistance = 30.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "deg"))
	float WallRunMaxSurfaceTilt = 15.f;
	
	/** Fraction of upward speed kept when attaching (falling speed is always caught at 0) */
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallRunUpSpeedCarry = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s", ClampMin = "0.0"))
	float WallRunMaxEntryUpSpeed = 250.f;

	/** Gravity ramps from Start to End over WallRunMaxDuration, shaped by the exponent (higher = floatier, then a sharper drop) */
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.0"))
	float WallRunGravityScaleStart = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.0"))
	float WallRunGravityScaleEnd = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.1"))
	float WallRunGravityCurveExponent = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s"))
	float WallRunStickSpeed = 200.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "s"))
	float WallRunMaxDuration = 1.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallRunSteerAwayThreshold = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s"))
	float WallJumpOutSpeed = 500.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s"))
	float WallJumpUpSpeed = 500.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "cm"))
	float WallKickTraceDistance = 20.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "cm"))
	float WallKickTraceRadius = 20.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "deg"))
	float WallKickMaxAngle = 45.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "cm/s"))
	float WallKickOutSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "cm/s"))
	float WallKickUpSpeed = 600.f;

	/** Air control multiplier during a kick flight; input only acts along the kick direction (stretch or shorten, no steering). 0 = full lock */
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallKickAirControl = 1.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float GrappleCooldown = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ClampMin = "0.0"))
	float GrappleGravityScale = 1.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float GrappleReelFraction = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "cm/s", ClampMin = "0.0"))
	float GrappleReelSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "cm", ClampMin = "0.0"))
	float GrappleMinRopeLength = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ClampMin = "0.0"))
	float GrappleSwingControl = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float GrappleFloorGraceTime = 0.2f;

	/** In the air after a wall kick: no air braking, scaled air control, facing the flight direction */
	bool bIsWallKickFlight = false;
	
	TWeakObjectPtr<const UPrimitiveComponent> CurrentWall;
	TWeakObjectPtr<const UPrimitiveComponent> LastWall;
	TWeakObjectPtr<const AActor> GrappleAnchorActor;
	
	FVector WallRunNormal = FVector::ZeroVector;
	FVector GrappleAnchor = FVector::ZeroVector;
	
	float WallRunStartTime = 0.f;
	
	float RopeLength = 0.f;
	float TargetRopeLength = 0.f;
	
	float GrappleStartTime = 0.f;
	float GrappleEndTime = -1000.f;
	
public:
	UMPCharacterMovementComponent();
	
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual bool IsMovingOnGround() const override;
	
	virtual bool CanAttemptJump() const override;
	
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	
	bool IsCustomMovementMode(EMPCustomMovementMode Mode) const;
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	bool IsSliding() const;
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	bool IsWallRunning() const;
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	float GetWallRunSide() const;
	
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

	virtual float GetMaxBrakingDeceleration() const override;

	virtual FVector GetAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration) override;

	static FString MovementModeToString(EMovementMode Mode, uint8 CustomMode);

	static bool IsDebugEnabled();
	
	void SetWantsToSlide(bool bWants);

	/** Would a standing capsule fit here, keeping the feet where they are? */
	bool CanStandUp() const;
	
	bool CanWallKick() const;
	
	bool LastJumpWasWallKick() const { return bLastJumpWasWallKick; }

	// Grappling API
	void RequestGrapple(const FVector& Anchor, const AActor* AnchorActor);
	
	void ReleaseGrapple();
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	bool IsGrappling() const;
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	float GetGrappleSwingAngle() const;
	
	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	FVector GetGrappleAnchor() const { return GrappleAnchor; }
	
	const AActor* GetGrappleAnchorActor() const { return GrappleAnchorActor.Get(); }
	
protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	
	void PhysSlide(float deltaTime, int32 Iterations);
	
	void PhysWallRun(float deltaTime, int32 Iterations);
	
	void PhysGrapple(float deltaTime, int32 Iterations);
	
	bool ShouldReleaseGrapple() const;
	
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	bool FindRunnableWall(FHitResult& OutWallHit) const;
	
	bool FindKickableWall(FHitResult& OutKickHit) const;
	
	void ApplyRopeToVelocity();
	
	void ApplyRopeToPosition();
};
