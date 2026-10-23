// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Effect/EffectBase.h"
#include "DeleteEffect.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WILDCARD_API UDeleteEffect : public UEffectBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData) override;
	
};
