// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MPCharacterMovementComponent.generated.h"

enum class EMPCustomMovementMode : uint8;
enum class EMPDebugKey : int32;

DECLARE_LOG_CATEGORY_EXTERN(LogMPMovement, Log, All);

UCLASS()
class MOVEMENTPLAYGROUND_API UMPCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UMPCharacterMovementComponent();

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual bool IsMovingOnGround() const override;

	virtual bool CanAttemptJump() const override;

	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;

	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation) const override;

	virtual float GetMaxBrakingDeceleration() const override;

	virtual FVector GetAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration) override;

	void ResetMovementState();

	bool IsCustomMovementMode(EMPCustomMovementMode Mode) const;

	static FString MovementModeToString(EMovementMode Mode, uint8 CustomMode);

	static bool IsDebugEnabled();

	static void PrintDebugMessage(EMPDebugKey Key, const FString& Message);

	void SetWantsToSlide(bool bWants);

	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	bool IsSliding() const;

	bool CanStandUp() const;

	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	bool IsWallRunning() const;

	UFUNCTION(BlueprintPure, Category = "MP|Movement")
	float GetWallRunSide() const;

	bool CanWallKick() const;

	bool LastJumpWasWallKick() const { return bLastJumpWasWallKick; }

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

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	void ExitPhysicsTo(const EMovementMode Mode, const float RemainingTime, const int32 Iterations);

	void MoveAndSlide(const FVector& Delta, float TimeTick);

	void TryEnterSlide();

	void OnEnterSlide();

	void OnExitSlide();

	void PhysSlide(float deltaTime, int32 Iterations);

	void TryEnterWallRun();

	void OnEnterWallRun();

	void OnExitWallRun();

	void PhysWallRun(float deltaTime, int32 Iterations);

	[[nodiscard]] float GetWallRunTraceLength() const;

	[[nodiscard]] TOptional<FHitResult> TraceWall(const FVector& Start, const FVector& End) const;

	[[nodiscard]] TOptional<FHitResult> TraceCurrentWall() const;

	[[nodiscard]] bool IsTooLowForWallRun() const;

	[[nodiscard]] bool ShouldLeaveWallRun() const;

	[[nodiscard]] float GetWallRunGravityScale() const;

	void UpdateWallRunVelocity(float TimeTick);

	[[nodiscard]] bool IsWallSurface(const FVector& Normal) const;

	[[nodiscard]] TOptional<FHitResult> FindRunnableWall() const;

	void PerformWallJump();

	[[nodiscard]] TOptional<FHitResult> FindKickableWall() const;

	void PerformWallKick(const FHitResult& KickHit);

	void TryEnterGrapple();

	void OnEnterGrapple();

	void OnExitGrapple();

	void PhysGrapple(float deltaTime, int32 Iterations);

	[[nodiscard]] bool ShouldReleaseGrapple() const;

	void UpdateGrappleReel(float TimeTick);

	void ApplyGrappleSwingInput(float TimeTick);

	void ApplyRopeToVelocity();

	void ApplyRopeToPosition();

private:
	void EvalPreviousCustomMovementMode(const EMPCustomMovementMode Mode);

	void EvalCurrentCustomMovementMode(const EMPCustomMovementMode Mode);

	void LogMovementModeTransition(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) const;

	[[nodiscard]] static FColor GetDebugTraceColor(bool bHit, bool bAccepted);

	void DrawDebugTraceLine(const FVector& Start, const FVector& End, const FColor& Color) const;

	void DrawDebugTraceSphere(const FVector& Center, float Radius, const FColor& Color) const;

private:
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

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallRunUpSpeedCarry = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Wall Run", meta = (ForceUnits = "cm/s", ClampMin = "0.0"))
	float WallRunMaxEntryUpSpeed = 250.f;

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

	bool bWantsToSlide = false;

	bool bHasWallCandidate = false;
	FVector WallRunNormal = FVector::ZeroVector;
	float WallRunStartTime = 0.f;
	TWeakObjectPtr<const UPrimitiveComponent> CurrentWall;
	TWeakObjectPtr<const UPrimitiveComponent> LastWall;

	bool bHasKickCandidate = false;
	bool bLastJumpWasWallKick = false;

	bool bIsWallKickFlight = false;

	bool bWantsToGrapple = false;
	FVector GrappleAnchor = FVector::ZeroVector;
	TWeakObjectPtr<const AActor> GrappleAnchorActor;
	float RopeLength = 0.f;
	float TargetRopeLength = 0.f;
	float GrappleStartTime = 0.f;
	float GrappleEndTime = -1000.f;
};
