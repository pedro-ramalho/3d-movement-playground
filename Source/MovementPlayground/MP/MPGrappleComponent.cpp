// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPGrappleComponent.h"
#include "MP/MPCharacterMovementComponent.h"
#include "MP/MPGrapplePoint.h"
#include "MP/MPMovementTypes.h"

#include "CableComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"

UMPGrappleComponent::UMPGrappleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMPGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());

	if (!Character)
	{
		return;
	}

	Movement = Cast<UMPCharacterMovementComponent>(Character->GetCharacterMovement());
	Camera = Character->FindComponentByClass<UCameraComponent>();
	GrappleCable = Character->FindComponentByClass<UCableComponent>();

	Character->MovementModeChangedDelegate.AddDynamic(this, &UMPGrappleComponent::OnMovementModeChanged);
}

void UMPGrappleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Character)
	{
		Character->MovementModeChangedDelegate.RemoveDynamic(this, &UMPGrappleComponent::OnMovementModeChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UMPGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Character || !Movement || !Camera)
	{
		return;
	}

	UpdateGrappleShot();
	UpdateGrappleTarget();
}

void UMPGrappleComponent::StartGrapple()
{
	if (!Movement)
	{
		return;
	}

	if (AMPGrapplePoint* Target = GrappleTarget.Get())
	{
		Movement->RequestGrapple(Target->GetAnchorLocation(), Target);
	}
}

void UMPGrappleComponent::StopGrapple()
{
	if (Movement)
	{
		Movement->ReleaseGrapple();
	}
}

void UMPGrappleComponent::OnMovementModeChanged(ACharacter* InCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (Movement && Movement->IsGrappling())
	{
		StartGrappleShot();
	}

	if (PrevMovementMode == MOVE_Custom && static_cast<EMPCustomMovementMode>(PreviousCustomMode) == EMPCustomMovementMode::Grapple)
	{
		HideGrappleCable();
	}
}

void UMPGrappleComponent::UpdateGrappleTarget()
{
	AMPGrapplePoint* NewTarget = FindBestGrapplePoint();
	AMPGrapplePoint* OldTarget = GrappleTarget.Get();

	if (NewTarget != OldTarget)
	{
		if (OldTarget)
		{
			OldTarget->SetHighlighted(false);
		}

		if (NewTarget)
		{
			NewTarget->SetHighlighted(true);
		}

		GrappleTarget = NewTarget;
	}

	if (UMPCharacterMovementComponent::IsDebugEnabled())
	{
		UMPCharacterMovementComponent::PrintDebugMessage(EMPDebugKey::GrappleTarget,
			FString::Printf(TEXT("Grapple target: %s"), NewTarget ? *NewTarget->GetName() : TEXT("none")));
	}
}

AMPGrapplePoint* UMPGrappleComponent::FindBestGrapplePoint() const
{
	const FVector CharacterCenter = Character->GetActorLocation();

	TArray<FOverlapResult> Candidates;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FindBestGrapplePoint), false, Character);

	GetWorld()->OverlapMultiByChannel(Candidates, CharacterCenter, FQuat::Identity, ECC_Grapple,
		FCollisionShape::MakeSphere(GrappleMaxRange), Params);

	const float MinAimDot = FMath::Cos(FMath::DegreesToRadians(GrappleAimAngle));

	const FVector CameraLocation = Camera->GetComponentLocation();
	const FVector CameraForward = Camera->GetForwardVector();

	AMPGrapplePoint* BestPoint = nullptr;
	float BestDot = -1.f;

	FCollisionQueryParams LineParams(SCENE_QUERY_STAT(GrappleLineOfSight), false, Character);

	const bool bDebug = UMPCharacterMovementComponent::IsDebugEnabled();
	TArray<TPair<FVector, FColor>, TInlineAllocator<16>> DebugLines;

	for (const FOverlapResult& Candidate : Candidates)
	{
		AMPGrapplePoint* GrapplePoint = Cast<AMPGrapplePoint>(Candidate.GetActor());

		if (!GrapplePoint)
		{
			continue;
		}

		if (Movement->IsGrappling() && GrapplePoint == Movement->GetGrappleAnchorActor())
		{
			continue;
		}

		const FVector AnchorLocation = GrapplePoint->GetAnchorLocation();

		const FVector ToPoint = (AnchorLocation - CameraLocation).GetSafeNormal();
		const float AimDot = FVector::DotProduct(CameraForward, ToPoint);

		if (AimDot < MinAimDot)
		{
			if (bDebug)
			{
				DebugLines.Emplace(AnchorLocation, FColor::Silver);
			}

			continue;
		}

		const bool bTooLow = AnchorLocation.Z < CharacterCenter.Z + GrappleMinHeightAbove;

		FHitResult BlockingHit;
		const bool bBlocked = !bTooLow && GetWorld()->LineTraceSingleByChannel(
			BlockingHit, CharacterCenter, AnchorLocation, ECC_Visibility, LineParams
		);

		if (bTooLow || bBlocked)
		{
			if (bDebug)
			{
				DebugLines.Emplace(AnchorLocation, FColor::Red);
			}

			continue;
		}

		if (bDebug)
		{
			DebugLines.Emplace(AnchorLocation, FColor::Yellow);
		}

		if (AimDot > BestDot)
		{
			BestDot = AimDot;
			BestPoint = GrapplePoint;
		}
	}

#if !UE_BUILD_SHIPPING
	if (bDebug)
	{
		for (const TPair<FVector, FColor>& Line : DebugLines)
		{
			DrawDebugLine(GetWorld(), CharacterCenter, Line.Key, Line.Value, false, -1.f, 0, 1.5f);
		}

		if (BestPoint)
		{
			DrawDebugLine(GetWorld(), CharacterCenter, BestPoint->GetAnchorLocation(), FColor::Green, false, -1.f, 0, 2.5f);
		}
	}
#endif

	return BestPoint;
}

void UMPGrappleComponent::StartGrappleShot()
{
	if (!GrappleCable)
	{
		return;
	}

	const AActor* AnchorActor = Movement->GetGrappleAnchorActor();
	USceneComponent* AnchorComponent = AnchorActor ? AnchorActor->GetRootComponent() : nullptr;

	if (!AnchorComponent)
	{
		return;
	}

	GrappleCable->SetAttachEndToComponent(AnchorComponent);
	GrappleCable->EndLocation = AnchorComponent->GetComponentTransform().InverseTransformPosition(GrappleCable->GetComponentLocation());
	GrappleCable->SetVisibility(true);

	GrappleShotStartTime = GetWorld()->GetTimeSeconds();
}

void UMPGrappleComponent::UpdateGrappleShot()
{
	if (GrappleShotStartTime < 0.f || !GrappleCable)
	{
		return;
	}

	const AActor* AnchorActor = Movement->GetGrappleAnchorActor();
	const USceneComponent* AnchorComponent = AnchorActor ? AnchorActor->GetRootComponent() : nullptr;

	const float ShotAlpha = GrappleShotDuration > 0.f
		? FMath::Clamp((GetWorld()->GetTimeSeconds() - GrappleShotStartTime) / GrappleShotDuration, 0.f, 1.f)
		: 1.f;

	if (!AnchorComponent || ShotAlpha >= 1.f)
	{
		GrappleCable->EndLocation = FVector::ZeroVector;
		GrappleShotStartTime = -1.f;

		return;
	}

	const float EasedAlpha = FMath::InterpEaseOut(0.f, 1.f, ShotAlpha, 2.f);
	const FVector EndWorld = FMath::Lerp(GrappleCable->GetComponentLocation(), AnchorComponent->GetComponentLocation(), EasedAlpha);

	GrappleCable->EndLocation = AnchorComponent->GetComponentTransform().InverseTransformPosition(EndWorld);
}

void UMPGrappleComponent::HideGrappleCable()
{
	if (!GrappleCable)
	{
		return;
	}

	GrappleCable->SetVisibility(false);
	GrappleShotStartTime = -1.f;
}
