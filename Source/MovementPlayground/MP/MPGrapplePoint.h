// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MPGrapplePoint.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

/**
 * Grapple anchor placed in the level. Only answers Grapple traces; the character does the aiming and highlighting.
 */
UCLASS()
class MOVEMENTPLAYGROUND_API AMPGrapplePoint : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple")
	TObjectPtr<UMaterialInterface> NormalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	bool bIsHighlighted = false;

public:
	AMPGrapplePoint();

	void SetHighlighted(bool bInHighlighted);

	FVector GetAnchorLocation() const { return GetActorLocation(); }
};
