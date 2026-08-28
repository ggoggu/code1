// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/WildEnemyHandWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "WildHUDViewModel.h"

void UWildEnemyHandWidget::InitializeEnemyHandWidget(UWildHUDViewModel* InViewModel)
{
	if (!InViewModel) return;

	if (CachedViewModel && EnemyHandCountChangedHandle.IsValid())
	{
		CachedViewModel->RemoveFieldValueChangedDelegate(
			UWildHUDViewModel::FFieldNotificationClassDescriptor::EnemyHandCount, 
			EnemyHandCountChangedHandle
		);
	}

	CachedViewModel = InViewModel;
	EnemyHandCountChangedHandle = CachedViewModel->AddFieldValueChangedDelegate(
		UWildHUDViewModel::FFieldNotificationClassDescriptor::EnemyHandCount,
		FFieldValueChangedDelegate::CreateUObject(this, &UWildEnemyHandWidget::OnEnemyHandCountUpdated_MVVM)
	);
	
	OnEnemyHandCountUpdated();
}

void UWildEnemyHandWidget::OnEnemyHandCountUpdated_MVVM(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	OnEnemyHandCountUpdated();
}

void UWildEnemyHandWidget::OnEnemyHandCountUpdated()
{
	if (!CachedViewModel || !HandCanvas || !EnemyCardWidgetClass) return;

	HandCanvas->ClearChildren();
	
	int32 EnemyCount = CachedViewModel->EnemyHandCount;

	for (int32 i = 0; i < EnemyCount; ++i)
	{
		UUserWidget* EnemyCard = CreateWidget<UUserWidget>(GetWorld(), EnemyCardWidgetClass);
		if (EnemyCard)
		{
			UCanvasPanelSlot* CanvasSlot = HandCanvas->AddChildToCanvas(EnemyCard);
			if (CanvasSlot)
			{
				CanvasSlot->SetAutoSize(true);
				// [핵심] 화면 위에서 아래로 내려오도록 앵커와 정렬을 상단 중앙으로 설정
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.0f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
			}
		}
	}
	UpdateCardLayout();
}

void UWildEnemyHandWidget::UpdateCardLayout()
{
	if (!HandCanvas) return;

	int32 CardCount = HandCanvas->GetChildrenCount();
	if (CardCount == 0) return;

	float TotalAngle = (CardCount - 1) * MaxAnglePerCard;
	float StartAngle = -(TotalAngle / 2.0f);
	float StartX = -((CardCount - 1) * CardSpacing / 2.0f);

	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		UWidget* ChildWidget = HandCanvas->GetChildAt(Index);
		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ChildWidget->Slot);
		
		if (CanvasSlot)
		{
			float CurrentAngle = StartAngle + (Index * MaxAnglePerCard);
			float CurrentX = StartX + (Index * CardSpacing);
			float Radian = FMath::DegreesToRadians(CurrentAngle);
			
			// 0.0(상단)에서 양수 방향이 아래쪽이므로 계산된 오프셋을 그대로 Y에 줍니다.
			float CurrentY = ArcRadius - (ArcRadius * FMath::Cos(Radian));

			// [핵심] 상단 중앙 기준 회전
			ChildWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));

			FWidgetTransform NewTransform;
			NewTransform.Translation = FVector2D(CurrentX, CurrentY);
			// 위에서 뻗어나오는 부채꼴이므로 회전 각도를 반대로 꺾어줍니다.
			NewTransform.Angle = -CurrentAngle; 
			
			ChildWidget->SetRenderTransform(NewTransform);
			CanvasSlot->SetZOrder(Index);
		}
	}
}
