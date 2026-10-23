// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WildInteractableInterface.generated.h"

class AInGameController;

UINTERFACE(MinimalAPI, Blueprintable)
class UWildInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 타일 밖 인터랙션 액터(재미 요소 등)나 클릭 가능한 오브젝트를 위한 인터페이스
 */
class WILDCARD_API IWildInteractableInterface
{
	GENERATED_BODY()

public:
	/** 마우스 클릭 상호작용 시 호출되는 함수 (C++ 및 블루프린트에서 오버라이드 가능) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(AInGameController* Controller);
};
