
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MPGrapplePoint.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class MOVEMENTPLAYGROUND_API AMPGrapplePoint : public AActor
{
	GENERATED_BODY()

public:
	AMPGrapplePoint();

	void SetHighlighted(bool bInHighlighted);

	FVector GetAnchorLocation() const { return GetActorLocation(); }

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple")
	TObjectPtr<UMaterialInterface> NormalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "MP|Grapple")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	bool bIsHighlighted = false;
};
