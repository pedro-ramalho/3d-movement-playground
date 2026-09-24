// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MPCharacter.h"
#include "MP/MPCharacterMovementComponent.h"

AMPCharacter::AMPCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UMPCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	
	MPMovement = CastChecked<UMPCharacterMovementComponent>(GetCharacterMovement());
}

// Called to bind functionality to input
void AMPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
