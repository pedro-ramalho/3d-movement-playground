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
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run")
	float WallRunGravityScale = 0.3f;
	
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

	/** After a kick, air braking is off and air control fades back in over this time */
	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Kick", meta = (ForceUnits = "s", ClampMin = "0.0"))
	float WallKickRecoveryTime = 0.3f;

	float WallKickTime = -1.f;
	
	TWeakObjectPtr<const UPrimitiveComponent> CurrentWall;
	TWeakObjectPtr<const UPrimitiveComponent> LastWall;
	
	FVector WallRunNormal = FVector::ZeroVector;
	
	float WallRunStartTime = 0.f;
	
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
	
	void SetWantsToSlide(bool bWants);

	/** Would a standing capsule fit here, keeping the feet where they are? */
	bool CanStandUp() const;
	
	bool CanWallKick() const;

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	
	void PhysSlide(float deltaTime, int32 Iterations);
	
	void PhysWallRun(float deltaTime, int32 Iterations);
	
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	bool FindRunnableWall(FHitResult& OutWallHit) const;
	
	bool FindKickableWall(FHitResult& OutKickHit) const;

	/** 0 at the moment of a wall kick, rising to 1 over WallKickRecoveryTime; 1 when no kick is recovering */
	float GetWallKickControlAlpha() const;
};
