// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Effect/EffectBase.h"
#include "IncreaseEffect.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WILDCARD_API UIncreaseEffect : public UEffectBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Increase Value")
	int32 SelectedNum = 0;
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData) override;
};
