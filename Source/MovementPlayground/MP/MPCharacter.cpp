// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MPCharacter.h"

#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Animation/AnimMontage.h"
#include "MP/MPMovementTypes.h"
#include "MP/MPCharacterMovementComponent.h"

const FName AMPCharacter::SlideGetUpNotifyName(TEXT("GetUp"));

AMPCharacter::AMPCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UMPCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	
	MPMovement = CastChecked<UMPCharacterMovementComponent>(GetCharacterMovement());
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	
	SetupCameraBoom();
	SetupFollowCamera();
}

void AMPCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &AMPCharacter::OnMontageNotifyBegin);
	}

	if (SlideMontage)
	{
		constexpr float SampleStep = 1.f / 30.f;
		const float Length = SlideMontage->GetPlayLength();

		for (float Start = 0.f; Start < Length; Start += SampleStep)
		{
			const float End = FMath::Min(Start + SampleStep, Length);
			const float SliceTime = End - Start;

			if (SliceTime < UE_KINDA_SMALL_NUMBER)
			{
				break;
			}

			const FTransform RootMotion = SlideMontage->ExtractRootMotionFromTrackRange(Start, End, FAnimExtractContext());
			const float SliceSpeed = RootMotion.GetTranslation().Size2D() / SliceTime * SlideMontage->RateScale;

			SlideMontagePeakSpeed = FMath::Max(SlideMontagePeakSpeed, SliceSpeed);
		}
	}
}

void AMPCharacter::OnMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (NotifyName != SlideGetUpNotifyName || !GetMPMovement()->IsSliding())
	{
		return;
	}

	if (GetMPMovement()->CanStandUp())
	{
		return;
	}

	StopAnimMontage(SlideMontage);
	GetMPMovement()->SetMovementMode(MOVE_Walking);
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

// Called to bind functionality to input
void AMPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMPCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMPCharacter::DoJumpEnd);
		
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AMPCharacter::DoSlideStart);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AMPCharacter::DoSlideEnd);
		
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMPCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMPCharacter::Look);
		
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMPCharacter::Look);
	}
}

void AMPCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	if (GetMPMovement()->IsSliding())
	{
		const float EntrySpeed = GetVelocity().Size2D();
		float RootMotionScale = 1.f;

		if (SlideMontagePeakSpeed > UE_KINDA_SMALL_NUMBER)
		{
			RootMotionScale = FMath::Clamp(EntrySpeed * SlideSpeedMultiplier / SlideMontagePeakSpeed, SlideRootMotionScaleMin, SlideRootMotionScaleMax);
		}

		SetAnimRootMotionTranslationScale(RootMotionScale);

		UE_LOG(LogMPMovement, Log, TEXT("Slide entry: %.0f cm/s, root motion scale %.2f"), EntrySpeed, RootMotionScale);

		PlayAnimMontage(SlideMontage);
	}

	if (PrevMovementMode == MOVE_Custom && static_cast<EMPCustomMovementMode>(PreviousCustomMode) == EMPCustomMovementMode::Slide)
	{
		SetAnimRootMotionTranslationScale(1.f);

		if (GetCharacterMovement()->MovementMode == MOVE_Falling)
		{
			if (SlideMontage)
			{
				if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
				{
					if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(SlideMontage))
					{
						MontageInstance->PushDisableRootMotion();
					}
				}
				
				StopAnimMontage(SlideMontage);
			}
		}
	}
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

bool AMPCharacter::CanJumpInternal_Implementation() const
{
	if (GetMPMovement()->IsCustomMovementMode(EMPCustomMovementMode::Slide))
		return JumpIsAllowedInternal();
	
	return Super::CanJumpInternal_Implementation();
}
