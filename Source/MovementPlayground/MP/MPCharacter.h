// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MPCharacter.generated.h"

class UMPCharacterMovementComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimMontage;

struct FInputActionValue;

UCLASS()
class MOVEMENTPLAYGROUND_API AMPCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	TObjectPtr<UMPCharacterMovementComponent> MPMovement;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(EditDefaultsOnly, Category = "MP|Animation")
	TObjectPtr<UAnimMontage> SlideExitMontage;

	/** Camera Boom and Follow Camera Setup */
	void SetupCameraBoom();
	
	void SetupFollowCamera();
	
protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SlideAction;
	
	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;
	
	/** Called for movement */
	void Move(const FInputActionValue& Value);
	
	/** Called for looking */
	void Look(const FInputActionValue& Value);

	virtual bool CanJumpInternal_Implementation() const override;
public:
	// Sets default values for this character's properties
	AMPCharacter(const FObjectInitializer& ObjectInitializer);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	
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
	
	FORCEINLINE UMPCharacterMovementComponent* GetMPMovement() const { return MPMovement; }
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
