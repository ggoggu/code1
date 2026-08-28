// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "WildHandWidget.generated.h"

class UCanvasPanel;
class UWildCardWidget;
class UWildHUDViewModel;

UCLASS()
class WILDCARD_API UWildHandWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// UMG 에디터에서 바인딩할 캔버스 패널 (여기에 카드 위젯들이 자식으로 붙습니다)
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* HandCanvas;

	// 생성할 카드 위젯의 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Setup")
	TSubclassOf<UWildCardWidget> CardWidgetClass;

	// --- 부채꼴 레이아웃 세팅 값 ---
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float CardSpacing = 80.0f; // 카드 간격

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float ArcRadius = 800.0f; // 부채꼴 곡률 반지름 (클수록 완만함)

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float MaxAnglePerCard = 5.0f; // 카드 1장당 최대 회전 각도

	// 현재 바인딩된 HUD 뷰모델
	UPROPERTY()
	UWildHUDViewModel* CachedViewModel;

protected:
	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// 뷰모델을 주입받고 이벤트를 연동하는 함수
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void InitializeHandWidget(UWildHUDViewModel* InViewModel);

private:
	// 뷰모델의 MyHandCards 배열이 갱신될 때 호출될 콜백
	void OnHandCardsUpdated();

	void OnHandCardsUpdated_MVVM(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	FDelegateHandle HandCardsChangedHandle;

	// 실제 부채꼴 수학 계산 및 트랜스폼 적용
	void UpdateCardLayout();

	// 위젯별로 최종적으로 도달해야 할 목표 트랜스폼 저장
	TMap<UWildCardWidget*, FWidgetTransform> TargetTransforms;
	
	// 애니메이션 보간 속도 (높을수록 빠름)
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float AnimationSpeed = 15.0f;

	UPROPERTY()
	UWildCardWidget* DraggedCard;

	UPROPERTY()
	UWildCardWidget* HoveredCard;

	void HandleCardHovered(UWildCardWidget* Card);
	void HandleCardUnhovered(UWildCardWidget* Card);
	void HandleCardDragStarted(UWildCardWidget* Card);
	void HandleCardDragMoved(UWildCardWidget* Card, FVector2D ScreenPos);
	void HandleCardDragEnded(UWildCardWidget* Card, FVector2D ScreenPos);
	
};
