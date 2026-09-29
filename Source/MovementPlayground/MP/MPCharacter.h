// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "MPCharacter.generated.h"

class UMPCharacterMovementComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;
class UCableComponent;
class UMPGrappleComponent;

struct FInputActionValue;

UCLASS()
class MOVEMENTPLAYGROUND_API AMPCharacter : public ACharacter
{
	GENERATED_BODY()
	
	// Components
	UPROPERTY(Transient)
	TObjectPtr<UMPCharacterMovementComponent> MPMovement;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCableComponent> GrappleCable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMPGrappleComponent> GrappleComponent;

	// Animation Montages
	UPROPERTY(EditDefaultsOnly, Category = "MP|Animation")
	TObjectPtr<UAnimMontage> SlideMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Animation")
	TObjectPtr<UAnimMontage> WallKickMontage;

	UFUNCTION()
	void OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);
	
	// Slide-Specific Properties
	UPROPERTY(EditDefaultsOnly, Category = "MP|Slide")
	float SlideSpeedMultiplier = 1.2f;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Slide", meta = (ClampMin = "0.1"))
	float SlideRootMotionScaleMin = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Slide", meta = (ClampMin = "0.1"))
	float SlideRootMotionScaleMax = 2.5f;

	// Runtime State
	float SlideMontagePeakSpeed = 0.f;

	static const FName SlideGetUpNotifyName;

	// Setup Methods
	void SetupCameraBoom();

	void SetupFollowCamera();

	void SetupGrappleCable();

protected:
	// Overrides from ACharacter 
	virtual void BeginPlay() override;
	
	virtual bool CanJumpInternal_Implementation() const override;
	// End overrides from ACharacter
	
	// Input Actions
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SlideAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> GrappleAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;
	
	// Input Handlers
	void Move(const FInputActionValue& Value);
	
	void Look(const FInputActionValue& Value);

public:
	AMPCharacter(const FObjectInitializer& ObjectInitializer);

	// Start overrides from ACharacter
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	
	virtual void OnJumped_Implementation() override;
	// End overrides from ACharacter
	
	// Input Handlers
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSlideStart();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoSlideEnd();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoGrappleStart();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoGrappleEnd();
	
	// Getters
	FORCEINLINE UMPCharacterMovementComponent* GetMPMovement() const { return MPMovement; }
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
