// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WildHUDWidget.generated.h"

class UWildHUDViewModel;
class UWildHandWidget;

UCLASS()
class WILDCARD_API UWildHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// UMG 에디터에서 배치할 하위 손패 위젯 (이름이 반드시 'HandWidget'이어야 바인딩 됨)
	UPROPERTY(meta = (BindWidget))
	UWildHandWidget* HandWidget;

	// 이 HUD가 참조하는 최상위 통합 뷰모델
	UPROPERTY(BlueprintReadOnly, Category = "HUD|ViewModel")
	UWildHUDViewModel* HUDViewModel;

	UPROPERTY(meta = (BindWidget))
	class UWildEnemyHandWidget* EnemyHandWidget;

public:
	// 게임 시작 시 PlayerController에서 호출하여 뷰모델을 주입하는 함수
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeHUD(UWildHUDViewModel* InViewModel);
	
};
