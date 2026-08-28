// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "WildEnemyHandWidget.generated.h"

class UCanvasPanel;
class UWildHUDViewModel;

UCLASS()
class WILDCARD_API UWildEnemyHandWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* HandCanvas;

	// 카드 뒷면 이미지만 설정된 아주 단순한 UUserWidget 블루프린트를 할당합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Setup")
	TSubclassOf<UUserWidget> EnemyCardWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float CardSpacing = 70.0f; 

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float ArcRadius = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float MaxAnglePerCard = 5.0f;

	UPROPERTY()
	UWildHUDViewModel* CachedViewModel;

public:
	void InitializeEnemyHandWidget(UWildHUDViewModel* InViewModel);

private:
	void OnEnemyHandCountUpdated();
	void OnEnemyHandCountUpdated_MVVM(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	FDelegateHandle EnemyHandCountChangedHandle;

	void UpdateCardLayout();
	
};
