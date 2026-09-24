// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MPCharacter.generated.h"

class UMPCharacterMovementComponent;

UCLASS()
class MOVEMENTPLAYGROUND_API AMPCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	TObjectPtr<UMPCharacterMovementComponent> MPMovement;
	
public:
	// Sets default values for this character's properties
	AMPCharacter(const FObjectInitializer& ObjectInitializer);

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	FORCEINLINE UMPCharacterMovementComponent* GetMPMovement() const { return MPMovement; }
};
