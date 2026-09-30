// Copyright Epic Games, Inc. All Rights Reserved.


#include "MovementPlaygroundPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "MovementPlayground.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MP/MPCharacter.h"
#include "MP/MPCourse.h"

void AMovementPlaygroundPlayerController::RespawnAtCheckpoint()
{
	if (Course)
	{
		RespawnAt(Course->GetRespawnTransform());
	}
}

void AMovementPlaygroundPlayerController::RestartRun()
{
	if (!Course)
	{
		return;
	}

	Course->ResetRun();
	RespawnAt(Course->GetRespawnTransform());
}

void AMovementPlaygroundPlayerController::BeginPlay()
{
	Super::BeginPlay();

	Course = Cast<AMPCourse>(UGameplayStatics::GetActorOfClass(this, AMPCourse::StaticClass()));

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogMovementPlayground, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AMovementPlaygroundPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (RestartAction)
		{
			EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Started, this, &AMovementPlaygroundPlayerController::RestartRun);
		}
	}
}

bool AMovementPlaygroundPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AMovementPlaygroundPlayerController::RespawnAt(const FTransform& SpawnTransform)
{
	if (AMPCharacter* MPCharacter = GetPawn<AMPCharacter>())
	{
		MPCharacter->Respawn(SpawnTransform);
	}
}
