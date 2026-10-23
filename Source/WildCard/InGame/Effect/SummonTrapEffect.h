// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Effect/EffectBase.h"
#include "SummonTrapEffect.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API USummonTrapEffect : public UEffectBase
{
	GENERATED_BODY()
public:
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap Setting")
	TSubclassOf<class AWebTrap> ClassToSpawn;
};
