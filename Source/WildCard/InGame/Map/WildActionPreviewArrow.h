// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WildActionPreviewArrow.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS()
class WILDCARD_API AWildActionPreviewArrow : public AActor
{
	GENERATED_BODY()

public:
	AWildActionPreviewArrow();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Preview|Arrow")
	void SetupArrow(const FVector& StartLocation, const FVector& EndLocation, const FLinearColor& InColor = FLinearColor(0.15f, 0.75f, 1.0f, 1.0f));

	UFUNCTION(BlueprintCallable, Category = "Preview|Arrow")
	void SetArrowColor(const FLinearColor& InColor);

	UFUNCTION(BlueprintCallable, Category = "Preview|Arrow")
	void ClearArrow();

protected:
	virtual void BeginPlay() override;

	void UpdateCurveGeometry();
	void EnsureMaterial();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> InstancedSegmentsComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ArrowHeadMeshComp;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	FLinearColor BaseColor = FLinearColor(0.15f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float ArcHeightMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float MinArcHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float MaxArcHeight = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float TubeRadius = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float ArrowHeadRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float ArrowHeadLength = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	int32 ActiveSegmentCount = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Arc")
	float DefaultHeightOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Performance")
	bool bUseMaterialShaderPulse = false;

	float PulseTimer = 0.0f;

	FVector CachedStart = FVector::ZeroVector;
	FVector CachedEnd = FVector::ZeroVector;
};
