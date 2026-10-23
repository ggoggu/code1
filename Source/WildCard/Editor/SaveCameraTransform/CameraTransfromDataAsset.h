// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CameraTransfromDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class WILDCARD_API UCameraTransfromDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Tool")
	TArray<FTransform> SavedTransforms;
};
