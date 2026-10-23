// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BattleData.h"
#include "EffectBase.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class WILDCARD_API UEffectBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Setting")
	EWildEffectTriggerTiming TriggerTiming = EWildEffectTriggerTiming::Instant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Setting")
	int32 Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect Setting")
	int32 Range = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "Card Effect")
	void ExecuteEffect(const FWildActionData& ActionData, const FEffectData& InEffectData);
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData) PURE_VIRTUAL(UWildCardEffectBase::ExecuteEffect_Implementation, );

	UFUNCTION(BlueprintNativeEvent, Category = "Card Effect")
	void ExecuteEffectTest(const FEffectData& InEffectData, UCardBase* InTarget, UCardBase* InSource, APlayerController* InPlayerController);
	virtual void ExecuteEffectTest_Implementation(const FEffectData& InEffectData, UCardBase* InTarget, UCardBase* InSource, APlayerController* InPlayerController) PURE_VIRTUAL(UWildCardEffectBase::ExecuteEffect_Implementation, );
	
	virtual UWorld* GetWorld() const override;

	/** 서버에서 클라이언트로 시각/UI 연출 브로드캐스트 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Card Effect|Visual")
	void BroadcastVisualCue(const FWildVisualCueData& CueData);
};

