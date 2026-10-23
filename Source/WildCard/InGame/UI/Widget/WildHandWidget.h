// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "InGame/UI/Widget/WildCardSelectionTypes.h"
#include "WildHandWidget.generated.h"

class UCanvasPanel;
class UWildCardWidget;
class UWildHUDViewModel;

DECLARE_DELEGATE_OneParam(FOnSearchLibraryCardSelected, class UWildCardWidget*);
DECLARE_DELEGATE(FOnSearchLibraryCancelled);

UCLASS()
class WILDCARD_API UWildHandWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// UMG 에디터에서 바인딩할 캔버스 패널 (여기에 카드 위젯들이 자식으로 붙습니다)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> HandCanvas;

	// 생성할 카드 위젯의 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Setup")
	TSubclassOf<UWildCardWidget> CardWidgetClass;

	// --- 부채꼴 레이아웃 세팅 값 ---
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float CardSpacing = 65.0f; // 카드 간격 (크기 축소에 맞춰 조정)

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float ArcRadius = 800.0f; // 부채꼴 곡률 반지름 (클수록 완만함)

	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float MaxAnglePerCard = 5.0f; // 카드 1장당 최대 회전 각도

	// 기본 카드 크기 배율
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float BaseCardScale = 0.8f;

	// 마우스 호버 시 카드 크기 배율
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float HoveredCardScale = 1.05f;

	// 카드 드래그 시 축소 배율
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float DraggedCardScale = 0.6f;

	// 카드 드래그 시 반투명도 (0.0: 완전 투명 ~ 1.0: 불투명)
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DraggedCardOpacity = 0.5f;

	// 카드 하나를 선택(또는 드래그/호버)했을 때 다른 카드들의 반투명도 (기본값: 0.1f = 10%)
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UnselectedCardOpacity = 0.1f;

	// 비활성화(서고탐색 등으로 선택)된 카드의 투명도 (기본값: 0.6f)
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DisabledCardOpacity = 0.6f;

	// 카드에 마우스 호버 시에도 다른 카드들을 반투명하게 처리할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	bool bDimOnHover = true;

	// 손패가 안쪽으로 들어가(접혀) 있을 때의 기본 Y 오프셋 (양수일수록 화면 아래쪽으로 내려감)
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float RetractedOffsetY = 190.0f;

	// 카드를 클릭하여 위로 올라왔을(펼쳐졌을) 때의 Y 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float ExpandedOffsetY = 30.0f;

	// 호버 시 위로 올라올 높이
	UPROPERTY(EditDefaultsOnly, Category = "Hand|Layout")
	float HoverYOffset = 60.0f;

	// 손패가 위로 올라와 펼쳐져 있는지 여부 (기본: false, 안쪽으로 들어가 있음)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hand|State")
	bool bIsExpanded = false;

	// 현재 바인딩된 HUD 뷰모델
	UPROPERTY()
	TObjectPtr<UWildHUDViewModel> CachedViewModel;

protected:
	virtual void NativeConstruct() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
	// 뷰모델을 주입받고 이벤트를 연동하는 함수
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void InitializeHandWidget(UWildHUDViewModel* InViewModel);

	UFUNCTION(BlueprintCallable, Category = "Hand")
	bool IsHoveringCard() const { return HoveredCard != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Hand")
	bool IsDraggingCard() const { return DraggedCard != nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Hand")
	UWildCardWidget* GetSelectedCard() const { return SelectedCard; }

	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetSelectedCard(UWildCardWidget* InCard);

	UFUNCTION(BlueprintCallable, Category = "Hand")
	void ClearCardSelection();

	// 손패 펼치기(위로 올리기) / 접기(안쪽으로 넣기) 설정
	UFUNCTION(BlueprintCallable, Category = "Hand")
	void SetHandExpanded(bool bExpand);

	UFUNCTION(BlueprintCallable, Category = "Hand")
	void ToggleHandExpanded();

	UFUNCTION(BlueprintCallable, Category = "Hand")
	bool IsHandExpanded() const { return bIsExpanded; }

	// 현재 손패 상태에 따른 Y 오프셋 반환
	float GetCurrentHandOffsetY() const { return bIsExpanded ? ExpandedOffsetY : RetractedOffsetY; }

	// --- 범용 카드 선택 모드 ---
	UFUNCTION(BlueprintCallable, Category = "Hand|CardSelection")
	void StartCardSelectionMode(int32 InMinCount = 1, int32 InMaxCount = 1);

	UFUNCTION(BlueprintCallable, Category = "Hand|CardSelection")
	void EndCardSelectionMode();

	UFUNCTION(BlueprintCallable, Category = "Hand|CardSelection")
	bool IsInCardSelectionMode() const { return CurrentInteractionMode == EHandInteractionMode::CardSelection; }

	UFUNCTION(BlueprintCallable, Category = "Hand|CardSelection")
	const TArray<UWildCardWidget*>& GetSelectedCardsForAction() const { return CurrentSelectedCards; }

	TArray<int32> GetSelectedCardInstanceIDs() const;

	FOnCardSelectionCountChanged OnCardSelectionCountChanged;
	FOnCardSelectionCancelled OnCardSelectionCancelledDelegate;

	// 하위 호환성 래퍼: 서고 탐색 (전술 스킬) 카드 선택 모드
	UFUNCTION(BlueprintCallable, Category = "Hand|SearchLibrary")
	void StartSearchLibraryMode();

	UFUNCTION(BlueprintCallable, Category = "Hand|SearchLibrary")
	void EndSearchLibraryMode();

	UFUNCTION(BlueprintCallable, Category = "Hand|SearchLibrary")
	bool IsInSearchLibraryMode() const { return IsInCardSelectionMode(); }

	UFUNCTION(BlueprintCallable, Category = "Hand|SearchLibrary")
	UWildCardWidget* GetSearchLibrarySelectedCard() const { return CurrentSelectedCards.Num() > 0 ? CurrentSelectedCards[0] : nullptr; }

	// 카드 비활성화(회색화) 관리
	UFUNCTION(BlueprintCallable, Category = "Hand|CardState")
	void SetCardInstanceDisabled(int32 InstanceID, bool bDisable);

	UFUNCTION(BlueprintPure, Category = "Hand|CardState")
	bool IsCardInstanceDisabled(int32 InstanceID) const;

	UFUNCTION(BlueprintCallable, Category = "Hand|CardState")
	void ClearAllDisabledCards();

	FOnSearchLibraryCardSelected OnSearchLibraryCardSelected;
	FOnSearchLibraryCancelled OnSearchLibraryCancelled;

private:
	// 비활성화 및 회색화된 카드 인스턴스 ID 목록
	UPROPERTY()
	TSet<int32> DisabledCardInstanceIDs;

	// 범용 카드 선택 상태
	UPROPERTY()
	EHandInteractionMode CurrentInteractionMode = EHandInteractionMode::Normal;

	int32 SelectionMinCount = 1;
	int32 SelectionMaxCount = 1;

	UPROPERTY()
	TArray<TObjectPtr<UWildCardWidget>> CurrentSelectedCards;

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
	TObjectPtr<UWildCardWidget> DraggedCard;

	UPROPERTY()
	TObjectPtr<UWildCardWidget> HoveredCard;

	UPROPERTY()
	TObjectPtr<UWildCardWidget> SelectedCard;

	UWildCardWidget* GetActiveCard() const;

	void HandleCardClicked(UWildCardWidget* Card);
	void HandleCardHovered(UWildCardWidget* Card);
	void HandleCardUnhovered(UWildCardWidget* Card);
	void HandleCardDragStarted(UWildCardWidget* Card);
	void HandleCardDragMoved(UWildCardWidget* Card, FVector2D ScreenPos);
	void HandleCardDragEnded(UWildCardWidget* Card, FVector2D ScreenPos);
	
};
