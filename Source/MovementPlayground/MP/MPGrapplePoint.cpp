// Fill out your copyright notice in the Description page of Project Settings.

#include "MP/MPGrapplePoint.h"

#include "Components/StaticMeshComponent.h"

AMPGrapplePoint::AMPGrapplePoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetCollisionProfileName(TEXT("GrapplePoint"));
}

void AMPGrapplePoint::SetHighlighted(bool bInHighlighted)
{
	if (bInHighlighted == bIsHighlighted)
		return;

	bIsHighlighted = bInHighlighted;

	UMaterialInterface* Material = bIsHighlighted ? HighlightMaterial : NormalMaterial;

	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}
}
