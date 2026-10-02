// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MPCharacter.h"

#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Animation/AnimMontage.h"
#include "CableComponent.h"
#include "MP/MPMovementTypes.h"
#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPGrappleComponent.h"
#include "MovementPlaygroundPlayerController.h"

const FName AMPCharacter::SlideLoopSectionName(TEXT("Loop"));
const FName AMPCharacter::SlideExitSectionName(TEXT("Exit"));

AMPCharacter::AMPCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UMPCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	
	MPMovement = CastChecked<UMPCharacterMovementComponent>(GetCharacterMovement());
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	
	SetupCameraBoom();
	SetupFollowCamera();
	SetupGrappleCable();

	GrappleComponent = CreateDefaultSubobject<UMPGrappleComponent>(TEXT("GrappleComponent"));
}

// Called to bind functionality to input
void AMPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMPCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMPCharacter::DoJumpEnd);
		
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AMPCharacter::DoSlideStart);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMPCharacter::DoSlideEnd);
		
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Started, this, &AMPCharacter::DoGrappleStart);
		EnhancedInputComponent->BindAction(GrappleAction, ETriggerEvent::Completed, this, &AMPCharacter::DoGrappleEnd);
		
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMPCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMPCharacter::Look);
		
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMPCharacter::Look);
	}
}

void AMPCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UMPCharacterMovementComponent* Movement = GetMPMovement();

	if (Movement->IsSliding())
	{
		StartSlideMontage();
	}

	const bool bWallKickOver = Movement->MovementMode == MOVE_Walking || Movement->IsWallRunning() || Movement->IsGrappling();

	if (WallKickMontage && bWallKickOver)
	{
		StopAnimMontage(WallKickMontage);
	}

	const bool bWasSliding = PrevMovementMode == MOVE_Custom
		&& static_cast<EMPCustomMovementMode>(PreviousCustomMode) == EMPCustomMovementMode::Slide;

	if (bWasSliding)
	{
		EndSlideMontage();
	}

	if (Movement->MovementMode == MOVE_Walking && GetWorld()->GetTimeSeconds() <= JumpBufferEnd)
	{
		JumpBufferEnd = -1.f;
		Jump();
	}
}

void AMPCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	JumpBufferEnd = -1.f;
	
	if (GetMPMovement()->LastJumpWasWallKick() && WallKickMontage)
		PlayAnimMontage(WallKickMontage);
}

void AMPCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	if (AMovementPlaygroundPlayerController* PlayerController = GetController<AMovementPlaygroundPlayerController>())
	{
		PlayerController->RespawnAtCheckpoint();
		return;
	}

	Super::FellOutOfWorld(DmgType);
}

void AMPCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMPCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMPCharacter::DoJumpStart()
{
	JumpBufferEnd = GetWorld()->GetTimeSeconds() + JumpBufferTime;
	Jump();
}

void AMPCharacter::DoJumpEnd()
{
	StopJumping();
}

void AMPCharacter::DoSlideStart()
{
	GetMPMovement()->SetWantsToSlide(true);
}

void AMPCharacter::DoSlideEnd()
{
	GetMPMovement()->SetWantsToSlide(false);
}

void AMPCharacter::DoGrappleStart()
{
	GrappleComponent->StartGrapple();
}

void AMPCharacter::DoGrappleEnd()
{
	GrappleComponent->StopGrapple();
}

void AMPCharacter::Respawn(const FTransform& SpawnTransform)
{
	const FRotator SpawnRotation(0.f, SpawnTransform.Rotator().Yaw, 0.f);

	TeleportTo(SpawnTransform.GetLocation(), SpawnRotation);

	StopJumping();
	GetMPMovement()->ResetMovementState();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.f);
	}

	if (Controller)
	{
		Controller->SetControlRotation(SpawnRotation);
	}
}

bool AMPCharacter::CanJumpInternal_Implementation() const
{
	if (GetMPMovement()->IsCustomMovementMode(EMPCustomMovementMode::Slide))
		return JumpIsAllowedInternal();
	
	if (GetMPMovement()->CanWallKick())
		return true;

	if (GetMPMovement()->IsWithinCoyoteTime())
		return true;
	
	return Super::CanJumpInternal_Implementation();
}

void AMPCharacter::Move(const FInputActionValue &Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMPCharacter::Look(const FInputActionValue &Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AMPCharacter::SetupCameraBoom()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;
}

void AMPCharacter::SetupFollowCamera()
{
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMPCharacter::SetupGrappleCable()
{
	GrappleCable = CreateDefaultSubobject<UCableComponent>(TEXT("GrappleCable"));
	GrappleCable->SetupAttachment(GetMesh(), TEXT("hand_r"));

	GrappleCable->CableLength = 0.f;
	GrappleCable->NumSegments = 8;
	GrappleCable->SolverIterations = 4;
	GrappleCable->bEnableStiffness = true;
	GrappleCable->CableWidth = 3.f;
	GrappleCable->bAttachEnd = true;

	GrappleCable->SetVisibility(false);
}

void AMPCharacter::StartSlideMontage()
{
	if (!SlideMontage)
	{
		return;
	}

	PlayAnimMontage(SlideMontage);

	const int32 LoopIndex = SlideMontage->GetSectionIndex(SlideLoopSectionName);

	if (LoopIndex == INDEX_NONE)
	{
		return;
	}

	const float LoopStart = SlideMontage->GetAnimCompositeSection(LoopIndex).GetTime() / FMath::Max(SlideMontage->RateScale, UE_KINDA_SMALL_NUMBER);

	GetWorldTimerManager().SetTimer(SlidePoseTimer, this, &AMPCharacter::HoldSlidePose, FMath::Max(LoopStart, UE_KINDA_SMALL_NUMBER), false);
}

void AMPCharacter::HoldSlidePose()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(SlideMontage))
	{
		return;
	}

	const FName CurrentSection = AnimInstance->Montage_GetCurrentSection(SlideMontage);

	if (CurrentSection == SlideLoopSectionName)
	{
		AnimInstance->Montage_Pause(SlideMontage);
		return;
	}

	if (CurrentSection != SlideExitSectionName)
	{
		SlidePoseTimer = GetWorldTimerManager().SetTimerForNextTick(this, &AMPCharacter::HoldSlidePose);
	}
}

void AMPCharacter::EndSlideMontage()
{
	GetWorldTimerManager().ClearTimer(SlidePoseTimer);

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!SlideMontage || !AnimInstance || !AnimInstance->Montage_IsActive(SlideMontage))
	{
		return;
	}

	AnimInstance->Montage_Resume(SlideMontage);

	const bool bStandingUp = GetMPMovement()->MovementMode == MOVE_Walking;

	if (bStandingUp && SlideMontage->IsValidSectionName(SlideExitSectionName))
	{
		AnimInstance->Montage_JumpToSection(SlideExitSectionName, SlideMontage);
		return;
	}

	StopAnimMontage(SlideMontage);
}
