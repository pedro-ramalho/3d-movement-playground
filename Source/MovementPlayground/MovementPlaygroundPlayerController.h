// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MovementPlaygroundPlayerController.generated.h"

class AMPCourse;
class UInputAction;
class UInputMappingContext;
class UMPHUDWidget;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AMovementPlaygroundPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void RespawnAtCheckpoint();

	void RestartRun();
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void RespawnAt(const FTransform& SpawnTransform);

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> RestartAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> RespawnAction;

	UPROPERTY(Transient)
	TObjectPtr<AMPCourse> Course;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMPHUDWidget> HUDWidgetClass;
	
	UPROPERTY(Transient)
	TObjectPtr<UMPHUDWidget> HUDWidget;
};
