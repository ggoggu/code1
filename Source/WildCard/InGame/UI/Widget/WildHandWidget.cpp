// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/Widget/WildHandWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "WildCardWidget.h"
#include "WildCardViewModel.h"
#include "WildHUDViewModel.h"
#include "InGameController.h"
#include "Engine/Engine.h"

void UWildHandWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

FReply UWildHandWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (IsInCardSelectionMode())
		{
			OnCardSelectionCancelledDelegate.ExecuteIfBound();
			OnSearchLibraryCancelled.ExecuteIfBound();
			return FReply::Handled();
		}

		if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
		{
			PC->CancelTileAction();
		}

		if (SelectedCard)
		{
			SetSelectedCard(nullptr);
			return FReply::Handled();
		}

		// 카드가 아닌 손패 영역의 빈 공간 클릭 시 손패가 올라와 있다면 안쪽으로 넣음
		if (bIsExpanded)
		{
			SetHandExpanded(false);
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWildHandWidget::SetHandExpanded(bool bExpand)
{
	if (bIsExpanded != bExpand)
	{
		bIsExpanded = bExpand;
		if (!bIsExpanded)
		{
			HoveredCard = nullptr;
			SelectedCard = nullptr;
			if (IsInCardSelectionMode())
			{
				CurrentSelectedCards.Empty();
			}
		}
		UpdateCardLayout();
	}
}

void UWildHandWidget::ToggleHandExpanded()
{
	SetHandExpanded(!bIsExpanded);
}

void UWildHandWidget::SetSelectedCard(UWildCardWidget* InCard)
{
	if (InCard && InCard->IsCardDisabled())
	{
		InCard = nullptr;
	}

	if (SelectedCard != InCard)
	{
		SelectedCard = InCard;
		UpdateCardLayout();
	}
}

void UWildHandWidget::ClearCardSelection()
{
	SetSelectedCard(nullptr);
}

void UWildHandWidget::StartCardSelectionMode(int32 InMinCount, int32 InMaxCount)
{
	CurrentInteractionMode = EHandInteractionMode::CardSelection;
	SelectionMinCount = InMinCount;
	SelectionMaxCount = InMaxCount;
	CurrentSelectedCards.Empty();
	SelectedCard = nullptr;
	HoveredCard = nullptr;
	SetHandExpanded(true);
	UpdateCardLayout();
}

void UWildHandWidget::EndCardSelectionMode()
{
	CurrentInteractionMode = EHandInteractionMode::Normal;
	for (UWildCardWidget* CardWidget : CurrentSelectedCards)
	{
		if (CardWidget && CardWidget->GetViewModel() &&
			!DisabledCardInstanceIDs.Contains(CardWidget->GetViewModel()->InstanceID))
		{
			CardWidget->SetCardDisabled(false);
		}
	}
	CurrentSelectedCards.Empty();
	SelectedCard = nullptr;
	HoveredCard = nullptr;
	UpdateCardLayout();
}

void UWildHandWidget::StartSearchLibraryMode()
{
	StartCardSelectionMode(1, 1);
}

void UWildHandWidget::EndSearchLibraryMode()
{
	EndCardSelectionMode();
}

TArray<int32> UWildHandWidget::GetSelectedCardInstanceIDs() const
{
	TArray<int32> Result;
	for (const UWildCardWidget* Card : CurrentSelectedCards)
	{
		if (Card && Card->GetViewModel())
		{
			Result.Add(Card->GetViewModel()->InstanceID);
		}
	}
	return Result;
}

void UWildHandWidget::SetCardInstanceDisabled(int32 InstanceID, bool bDisable)
{
	if (bDisable)
	{
		DisabledCardInstanceIDs.Add(InstanceID);
	}
	else
	{
		DisabledCardInstanceIDs.Remove(InstanceID);
	}

	if (HandCanvas)
	{
		for (UWidget* Child : HandCanvas->GetAllChildren())
		{
			if (UWildCardWidget* CardWidget = Cast<UWildCardWidget>(Child))
			{
				if (CardWidget->GetViewModel() && CardWidget->GetViewModel()->InstanceID == InstanceID)
				{
					CardWidget->SetCardDisabled(bDisable);
				}
			}
		}
	}
}

bool UWildHandWidget::IsCardInstanceDisabled(int32 InstanceID) const
{
	return DisabledCardInstanceIDs.Contains(InstanceID);
}

void UWildHandWidget::ClearAllDisabledCards()
{
	DisabledCardInstanceIDs.Empty();
	if (HandCanvas)
	{
		for (UWidget* Child : HandCanvas->GetAllChildren())
		{
			if (UWildCardWidget* CardWidget = Cast<UWildCardWidget>(Child))
			{
				CardWidget->SetCardDisabled(false);
			}
		}
	}
}

UWildCardWidget* UWildHandWidget::GetActiveCard() const
{
	if (IsValid(DraggedCard) && !DraggedCard->IsCardDisabled())
	{
		return DraggedCard;
	}
	if (IsValid(SelectedCard) && !SelectedCard->IsCardDisabled())
	{
		return SelectedCard;
	}
	if (bDimOnHover && IsValid(HoveredCard) && !HoveredCard->IsCardDisabled())
	{
		return HoveredCard;
	}
	return nullptr;
}

void UWildHandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWildCardWidget* ActiveCard = GetActiveCard();

	for (auto& Pair : TargetTransforms)
	{
		UWildCardWidget* CardWidget = Pair.Key;
		FWidgetTransform TargetTransform = Pair.Value;

		if (IsValid(CardWidget) && CardWidget != DraggedCard) // 드래그 중인 카드는 예외
		{
			FWidgetTransform CurrentTransform = CardWidget->GetRenderTransform();

			// FMath::FInterpTo 를 이용한 부드러운 보간 (이동, 회전, 크기)
			FWidgetTransform NewTransform;
			NewTransform.Translation = FMath::Vector2DInterpTo(CurrentTransform.Translation, TargetTransform.Translation, InDeltaTime, AnimationSpeed);
			NewTransform.Angle = FMath::FInterpTo(CurrentTransform.Angle, TargetTransform.Angle, InDeltaTime, AnimationSpeed);
			NewTransform.Scale = FMath::Vector2DInterpTo(CurrentTransform.Scale, TargetTransform.Scale, InDeltaTime, AnimationSpeed);

			CardWidget->SetRenderTransform(NewTransform);

			// 카드가 하나 선택(또는 드래그/호버)되었을 때 다른 카드들은 UnselectedCardOpacity(기본 0.1f = 10%)로 반투명화
			float TargetOpacity = 1.0f;
			if (IsInCardSelectionMode())
			{
				if (CurrentSelectedCards.Num() > 0)
				{
					TargetOpacity = CurrentSelectedCards.Contains(CardWidget) ? 1.0f : 0.75f;
				}
				else
				{
					TargetOpacity = 1.0f;
				}
			}
			else if (CardWidget->IsCardDisabled())
			{
				// 서고탐색 등으로 비활성화된 카드는 반투명(DisabledCardOpacity, 기본 0.6f) 상태 유지
				TargetOpacity = DisabledCardOpacity;
			}
			else if (ActiveCard != nullptr)
			{
				const bool bIsHighlighted = (CardWidget == SelectedCard) ||
											(CardWidget == DraggedCard) ||
											(bDimOnHover && CardWidget == HoveredCard);
				TargetOpacity = bIsHighlighted ? 1.0f : UnselectedCardOpacity;
			}

			float CurrentOpacity = CardWidget->GetRenderOpacity();
			if (!FMath::IsNearlyEqual(CurrentOpacity, TargetOpacity, 0.005f))
			{
				float NewOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, AnimationSpeed);
				CardWidget->SetRenderOpacity(NewOpacity);
			}
			else if (CurrentOpacity != TargetOpacity)
			{
				CardWidget->SetRenderOpacity(TargetOpacity);
			}
		}
	}
}

void UWildHandWidget::InitializeHandWidget(UWildHUDViewModel* InViewModel)
{
	if (!InViewModel) return;

	// [추가됨] 2단계: 기존에 바인딩 된 이벤트가 있다면 해제 (안전장치)
	if (CachedViewModel && HandCardsChangedHandle.IsValid())
	{
		CachedViewModel->RemoveFieldValueChangedDelegate(
			UWildHUDViewModel::FFieldNotificationClassDescriptor::MyHandCards, 
			HandCardsChangedHandle
		);
	}

	CachedViewModel = InViewModel;

	// [추가됨] 2단계: 뷰모델의 MyHandCards 필드가 변경될 때 호출되도록 이벤트 바인딩
	HandCardsChangedHandle = CachedViewModel->AddFieldValueChangedDelegate(
		UWildHUDViewModel::FFieldNotificationClassDescriptor::MyHandCards,
		FFieldValueChangedDelegate::CreateUObject(this, &UWildHandWidget::OnHandCardsUpdated_MVVM)
	);
	
	OnHandCardsUpdated();
}

void UWildHandWidget::OnHandCardsUpdated()
{
	if (!CachedViewModel || !HandCanvas || !CardWidgetClass) return;

	HandCanvas->ClearChildren();
	DraggedCard = nullptr;
	HoveredCard = nullptr;
	SelectedCard = nullptr;

	const TArray<UWildCardViewModel*>& HandData = CachedViewModel->MyHandCards;
	
	// 현재 손패에 남아있지 않은 InstanceID는 Disabled 목록에서 정리
	TSet<int32> CurrentHandIDs;
	for (UWildCardViewModel* CardVM : HandData)
	{
		if (CardVM)
		{
			CurrentHandIDs.Add(CardVM->InstanceID);
		}
	}
	DisabledCardInstanceIDs = DisabledCardInstanceIDs.Intersect(CurrentHandIDs);

	for (UWildCardViewModel* CardVM : HandData)
	{
		UWildCardWidget* NewCardWidget = CreateWidget<UWildCardWidget>(GetWorld(), CardWidgetClass);
		if (NewCardWidget)
		{
			NewCardWidget->SetRenderOpacity(1.0f);

			// [수정됨] 2단계: 주석 처리되어 있던 뷰모델 주입 로직 복구
			NewCardWidget->SetViewModel(CardVM);

			// 비활성화 및 회색화 상태 복원
			if (CardVM && DisabledCardInstanceIDs.Contains(CardVM->InstanceID))
			{
				NewCardWidget->SetCardDisabled(true);
			}

			NewCardWidget->OnClicked.BindUObject(this, &UWildHandWidget::HandleCardClicked);
			NewCardWidget->OnHovered.BindUObject(this, &UWildHandWidget::HandleCardHovered);
			NewCardWidget->OnUnhovered.BindUObject(this, &UWildHandWidget::HandleCardUnhovered);
			NewCardWidget->OnDragStarted.BindUObject(this, &UWildHandWidget::HandleCardDragStarted);
			NewCardWidget->OnDragMoved.BindUObject(this, &UWildHandWidget::HandleCardDragMoved);
			NewCardWidget->OnDragEnded.BindUObject(this, &UWildHandWidget::HandleCardDragEnded);
			
			UCanvasPanelSlot* CanvasSlot = HandCanvas->AddChildToCanvas(NewCardWidget);

			if (CanvasSlot)
			{
				CanvasSlot->SetAutoSize(true);
				CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
			}
		}
	}
	UpdateCardLayout();
}

void UWildHandWidget::OnHandCardsUpdated_MVVM(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	OnHandCardsUpdated();
}

void UWildHandWidget::UpdateCardLayout()
{
	if (!HandCanvas) return;

	int32 CardCount = HandCanvas->GetChildrenCount();
	if (CardCount == 0) return;

	float TotalAngle = (CardCount - 1) * MaxAnglePerCard;
	float StartAngle = -(TotalAngle / 2.0f);
	float StartX = -((CardCount - 1) * CardSpacing / 2.0f);

	// 목표 데이터 비우기
	TargetTransforms.Empty();

	// 손패 상태(접힘/올라옴)에 따른 기본 Y 오프셋 결정
	float CurrentHandOffsetY = GetCurrentHandOffsetY();

	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		UWidget* ChildWidget = HandCanvas->GetChildAt(Index);
		UWildCardWidget* CardWidget = Cast<UWildCardWidget>(ChildWidget);
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ChildWidget->Slot);
		
		if (!CanvasSlot || !CardWidget) continue;

		if (CardWidget == DraggedCard)
		{
			CanvasSlot->SetZOrder(999);
			continue;
		}

		// 카드 선택 모드에서 선택된 카드는 화면 중앙으로 배치 (단일/다중 대칭 정렬)
		if (IsInCardSelectionMode() && CurrentSelectedCards.Contains(CardWidget))
		{
			float CenterY = -370.0f;
			if (UWorld* World = GetWorld())
			{
				FVector2D ViewportSize = FVector2D(1920.f, 1080.f);
				if (GEngine && GEngine->GameViewport)
				{
					GEngine->GameViewport->GetViewportSize(ViewportSize);
				}
				if (ViewportSize.Y > 0.0f)
				{
					CenterY = -(ViewportSize.Y * 0.5f) + 160.0f;
				}
			}

			int32 SelectedIdx = CurrentSelectedCards.IndexOfByKey(CardWidget);
			int32 TotalSelected = CurrentSelectedCards.Num();

			// 다중 선택 시 수평 오프셋 계산 (카드가 1장이면 OffsetX = 0)
			float SelectedSpacing = 160.0f;
			float OffsetStartX = -((TotalSelected - 1) * SelectedSpacing) * 0.5f;
			float OffsetX = OffsetStartX + (SelectedIdx * SelectedSpacing);

			CardWidget->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
			FWidgetTransform CenterTransform;
			CenterTransform.Translation = FVector2D(OffsetX, CenterY);
			CenterTransform.Angle = 0.0f;
			CenterTransform.Scale = FVector2D(1.35f, 1.35f);

			TargetTransforms.Add(CardWidget, CenterTransform);
			CanvasSlot->SetZOrder(250 + SelectedIdx);
			continue;
		}

		float CurrentAngle = StartAngle + (Index * MaxAnglePerCard);
		float CurrentX = StartX + (Index * CardSpacing);
		float Radian = FMath::DegreesToRadians(CurrentAngle);
		float CurrentY = ArcRadius - (ArcRadius * FMath::Cos(Radian)) + CurrentHandOffsetY;
		
		float FinalScale = BaseCardScale;
		int32 FinalZOrder = Index;

		// 손패가 위로 올라와 있을 때만 개별 카드 호버 및 선택 연출 적용
		if (bIsExpanded && !CardWidget->IsCardDisabled())
		{
			if (CardWidget == SelectedCard)
			{
				CurrentY -= HoverYOffset; // 선택 시 위로 올라올 높이
				CurrentAngle = 0.0f; // 선택 시 똑바로 서게 함
				FinalScale = HoveredCardScale; 
				FinalZOrder = 110;
			}
			else if (CardWidget == HoveredCard)
			{
				CurrentY -= HoverYOffset; // 마우스 오버 시 위로 올라올 높이
				CurrentAngle = 0.0f; // 호버 시 똑바로 서게 함
				FinalScale = HoveredCardScale; 
				FinalZOrder = 100;
			}
		}

		CardWidget->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));

		// 즉시 SetRenderTransform 하지 않고 목표 좌표만 맵에 저장
		FWidgetTransform TargetTransform;
		TargetTransform.Translation = FVector2D(CurrentX, CurrentY);
		TargetTransform.Angle = CurrentAngle;
		TargetTransform.Scale = FVector2D(FinalScale, FinalScale);
			
		TargetTransforms.Add(CardWidget, TargetTransform);
		
		CanvasSlot->SetZOrder(FinalZOrder);
	}
}

void UWildHandWidget::HandleCardClicked(UWildCardWidget* Card)
{
	if (!IsValid(Card)) return;
	if (Card->IsCardDisabled()) return;

	if (IsInCardSelectionMode())
	{
		if (CurrentSelectedCards.Contains(Card))
		{
			// 이미 선택된 카드를 다시 클릭하면 선택 해제 (토글)
			CurrentSelectedCards.Remove(Card);
			UpdateCardLayout();
			OnCardSelectionCountChanged.Broadcast(CurrentSelectedCards.Num());
			if (CurrentSelectedCards.Num() == 0)
			{
				OnCardSelectionCancelledDelegate.ExecuteIfBound();
				OnSearchLibraryCancelled.ExecuteIfBound();
			}
			return;
		}

		if (SelectionMaxCount == 1)
		{
			// 단일 선택 모드: 기존 선택 해제하고 새로 선택
			CurrentSelectedCards.Empty();
			CurrentSelectedCards.Add(Card);
			SelectedCard = nullptr;
			UpdateCardLayout();
			OnCardSelectionCountChanged.Broadcast(CurrentSelectedCards.Num());
			OnSearchLibraryCardSelected.ExecuteIfBound(Card);
		}
		else
		{
			// 다중 선택 모드: MaxCount 이내라면 추가
			if (CurrentSelectedCards.Num() < SelectionMaxCount)
			{
				CurrentSelectedCards.Add(Card);
				SelectedCard = nullptr;
				UpdateCardLayout();
				OnCardSelectionCountChanged.Broadcast(CurrentSelectedCards.Num());
				OnSearchLibraryCardSelected.ExecuteIfBound(Card);
			}
		}
		return;
	}

	// 안쪽으로 들어가 있던 상태에서 카드를 클릭하면 위로 올라오게 함
	if (!bIsExpanded)
	{
		SetHandExpanded(true);
	}

	if (SelectedCard == Card)
	{
		SetSelectedCard(nullptr);
	}
	else
	{
		SetSelectedCard(Card);
	}
}

void UWildHandWidget::HandleCardHovered(UWildCardWidget* Card)
{
	if (DraggedCard) return; // 드래그 중에는 호버 연산 무시
	if (!IsValid(Card) || Card->IsCardDisabled()) return;
	if (IsInCardSelectionMode() && CurrentSelectedCards.Num() > 0) return; // 카드 선택 상태에서는 호버 연산 무시

	// 손패가 위로 올라와 있을 때만 카드 호버 효과 활성화
	if (bIsExpanded)
	{
		HoveredCard = Card;
		UpdateCardLayout();
	}
}

void UWildHandWidget::HandleCardUnhovered(UWildCardWidget* Card)
{
	if (HoveredCard == Card)
	{
		HoveredCard = nullptr;
		UpdateCardLayout();
	}
}

void UWildHandWidget::HandleCardDragStarted(UWildCardWidget* Card)
{
	if (!IsValid(Card)) return;
	if (Card->IsCardDisabled()) return;
	if (IsInCardSelectionMode()) return; // 카드 선택 중에는 카드 드래그 방지

	// 카드를 드래그하기 시작할 때도 손패를 펼쳐진 상태로 만듦
	SetHandExpanded(true);

	DraggedCard = Card;
	HoveredCard = nullptr;
	SelectedCard = Card;

	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		PC->ClearTileHover();
		PC->FindCanSpawnTiles();
	}

	if (IsValid(Card))
	{
		FWidgetTransform Transform = Card->GetRenderTransform();
		Transform.Angle = 0.0f;
		Transform.Scale = FVector2D(DraggedCardScale, DraggedCardScale);
		Card->SetRenderTransform(Transform);
		Card->SetRenderOpacity(DraggedCardOpacity);
	}

	UpdateCardLayout();
}

void UWildHandWidget::HandleCardDragMoved(UWildCardWidget* Card, FVector2D ScreenPos)
{
	if (Card != DraggedCard || !HandCanvas) return;

	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot);
	if (CanvasSlot)
	{
		FVector2D LocalPos = USlateBlueprintLibrary::AbsoluteToLocal(HandCanvas->GetCachedGeometry(), ScreenPos);

		// 치우침(Offset) 완전 해결: 앵커와 정렬을 0.5(중앙)으로 맞추고 위치를 마우스에 동기화
		CanvasSlot->SetAnchors(FAnchors(0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(LocalPos);

		// 드래그 중엔 카드가 축소된 크기로 똑바로 서있게 설정
		FWidgetTransform Transform;
		Transform.Angle = 0.0f;
		Transform.Scale = FVector2D(DraggedCardScale, DraggedCardScale);
		Card->SetRenderTransform(Transform);

		// 마우스 아래 타일 실시간 감지 및 호버 색상 갱신
		if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
		{
			PC->UpdateTileHover(ScreenPos);
		}
	}
}

void UWildHandWidget::HandleCardDragEnded(UWildCardWidget* Card, FVector2D ScreenPos)
{
	DraggedCard = nullptr;
	SelectedCard = nullptr;

	// 레이캐스트 실행 및 호버 해제를 컨트롤러로 전달
	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		PC->ClearTileHover();
		PC->AttemptPlayCard(Card->GetViewModel(), ScreenPos);
		// Keep Spawnable state until AttemptPlayCard has validated the drop.
		PC->CancledCanSpawnTiles();
	}

	// 드롭 실패(타일 위가 아님) 시 손패로 되돌리기 위한 원상 복구
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	UpdateCardLayout();

	if (IsValid(Card) && !TargetTransforms.Contains(Card))
	{
		Card->SetRenderOpacity(1.0f);
	}
}
