// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BattleData.h"
#include "EffectBase.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UEffectBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Setting")
	EWildEffectTriggerTiming TriggerTiming = EWildEffectTriggerTiming::Instant;

	UFUNCTION(BlueprintNativeEvent, Category = "Card Effect")
	void ExecuteEffect(const FWildActionData& ActionData);
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData) PURE_VIRTUAL(UWildCardEffectBase::ExecuteEffect_Implementation, );

	UFUNCTION(BlueprintNativeEvent, Category = "Card Effect")
	void ExecuteEffectTest(const FEffectData& InEffectData, ACardBase* InTarget, ACardBase* InSource, APlayerController* InPlayerController);
	virtual void ExecuteEffectTest_Implementation(const FEffectData& InEffectData, ACardBase* InTarget, ACardBase* InSource, APlayerController* InPlayerController) PURE_VIRTUAL(UWildCardEffectBase::ExecuteEffect_Implementation, );
	
	virtual UWorld* GetWorld() const override;
	
};
