// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/WildHandWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "WildCardWidget.h"
#include "WildHUDViewModel.h"
#include "InGameController.h"

void UWildHandWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UWildHandWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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

	const TArray<UWildCardViewModel*>& HandData = CachedViewModel->MyHandCards;
	
	for (UWildCardViewModel* CardVM : HandData)
	{
		UWildCardWidget* NewCardWidget = CreateWidget<UWildCardWidget>(GetWorld(), CardWidgetClass);
		if (NewCardWidget)
		{
			// [수정됨] 2단계: 주석 처리되어 있던 뷰모델 주입 로직 복구
			NewCardWidget->SetViewModel(CardVM);

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

		float CurrentAngle = StartAngle + (Index * MaxAnglePerCard);
		float CurrentX = StartX + (Index * CardSpacing);
		float Radian = FMath::DegreesToRadians(CurrentAngle);
		float CurrentY = ArcRadius - (ArcRadius * FMath::Cos(Radian));
		
		float FinalScale = 1.0f;
		int32 FinalZOrder = Index;

		if (CardWidget == HoveredCard)
		{
			CurrentY -= 40.0f; // 마우스 오버 시 위로 올라올 높이
			CurrentAngle = 0.0f; // 호버 시 똑바로 서게 함 (원치 않으면 제거)
			FinalScale = 1.2f; 
			FinalZOrder = 100;
		}

		CardWidget->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));

		// [핵심 변경점] 즉시 SetRenderTransform 하지 않고 목표 좌표만 맵에 저장합니다.
		FWidgetTransform TargetTransform;
		TargetTransform.Translation = FVector2D(CurrentX, CurrentY);
		TargetTransform.Angle = CurrentAngle;
		TargetTransform.Scale = FVector2D(FinalScale, FinalScale);
			
		TargetTransforms.Add(CardWidget, TargetTransform);
		
		CanvasSlot->SetZOrder(FinalZOrder);
	}
}

void UWildHandWidget::HandleCardHovered(UWildCardWidget* Card)
{
	if (DraggedCard) return; // 드래그 중에는 호버 연산 무시
	HoveredCard = Card;
	UpdateCardLayout();
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
	DraggedCard = Card;
	HoveredCard = nullptr;
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

		// 드래그 중엔 카드가 똑바로 서있게 설정
		FWidgetTransform Transform;
		Transform.Angle = 0.0f;
		Transform.Scale = FVector2D(1.0f, 1.0f);
		Card->SetRenderTransform(Transform);
	}
}

void UWildHandWidget::HandleCardDragEnded(UWildCardWidget* Card, FVector2D ScreenPos)
{
	DraggedCard = nullptr;

	// 레이캐스트 실행을 위해 컨트롤러로 전달
	if (AInGameController* PC = Cast<AInGameController>(GetWorld()->GetFirstPlayerController()))
	{
		PC->AttemptPlayCard(Card->GetViewModel(), ScreenPos);
	}

	// 드롭 실패(타일 위가 아님) 시 손패로 되돌리기 위한 원상 복구
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	UpdateCardLayout();
}
