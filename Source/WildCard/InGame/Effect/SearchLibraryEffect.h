// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Effect/EffectBase.h"
#include "SearchLibraryEffect.generated.h"

/**
 * 마법사 전술 스킬 '서고 탐색' 효과: 패에서 선택한 카드를 덱으로 되돌리고 1장 드로우
 */
UCLASS(BlueprintType, Blueprintable)
class WILDCARD_API USearchLibraryEffect : public UEffectBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData) override;
};
