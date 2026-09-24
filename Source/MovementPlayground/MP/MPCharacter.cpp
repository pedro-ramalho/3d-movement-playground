// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MPCharacter.h"

// Sets default values
AMPCharacter::AMPCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMPCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

