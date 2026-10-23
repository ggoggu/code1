// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGame/Unit/UnitBase.h"
#include "WildPreviewGhostUnit.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

UCLASS()
class WILDCARD_API AWildPreviewGhostUnit : public AActor
{
	GENERATED_BODY()

public:
	AWildPreviewGhostUnit();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Preview|Ghost")
	void SetupGhostUnit(UTexture2D* CardTexture, UTexture2D* FrameTexture, const FLinearColor& InTintColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f), float InOpacity = 0.35f, float InDesaturation = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Preview|Ghost")
	void SetGhostOpacity(float InOpacity);

	UFUNCTION(BlueprintCallable, Category = "Preview|Ghost")
	void SetGhostDesaturation(float InDesaturation);

	// Billboard & Visual Facing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Billboard")
	bool bEnableFacingCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Billboard")
	EUnitBillboardMode BillboardMode = EUnitBillboardMode::FullSpherical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Billboard")
	FRotator BillboardRotationOffset = FRotator::ZeroRotator;

protected:
	virtual void BeginPlay() override;

	void UpdateFacingCamera();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> CardVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CardMeshComp;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	FVector CardMeshScale = FVector(0.03f, 0.7f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	FVector CardMeshOffset = FVector(0.0f, 0.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	FLinearColor TintColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Opacity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Desaturation = 1.0f;

	float PulseTimer = 0.0f;
};
