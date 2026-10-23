// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/Widget/WildCardWidget.h"
#include "WildCardViewModel.h"
#include "View/MVVMView.h"
#include "InGameController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UWildCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (bIsCardDisabled)
	{
		SetCardDisabled(true);
	}
}

void UWildCardWidget::SetViewModel(UWildCardViewModel* InViewModel)
{
	CardViewModel = InViewModel;

	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		// 이름("WildCardViewModel")은 UMG 에디터의 ViewModels 패널에서 설정한 이름과 동일해야 합니다.
		// 만약 에디터에서 다른 이름으로 지정하셨다면 그 이름으로 변경해 주세요.
		View->SetViewModel(FName("WildCardViewModel"), InViewModel);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("WildCardWidget에 UMVVMView 익스텐션이 없습니다. UMG 디자이너에서 ViewModels를 할당했는지 확인하세요."));
	}

	if (bIsCardDisabled)
	{
		SetCardDisabled(true);
	}
}

void UWildCardWidget::SetCardDisabled(bool bDisable)
{
	bIsCardDisabled = bDisable;
	SetIsEnabled(!bDisable);

	if (bDisable)
	{
		bIsPressed = false;
		bIsDragging = false;
		if (HasMouseCapture())
		{
			FReply::Handled().ReleaseMouseCapture();
		}
	}

	const FLinearColor CardColor = bDisable ? FLinearColor(0.4f, 0.4f, 0.4f, 1.0f) : FLinearColor::White;
	SetColorAndOpacity(CardColor);

	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([bDisable, CardColor](UWidget* ChildWidget)
		{
			if (UImage* Image = Cast<UImage>(ChildWidget))
			{
				Image->SetColorAndOpacity(CardColor);
			}
			else if (UTextBlock* Text = Cast<UTextBlock>(ChildWidget))
			{
				Text->SetColorAndOpacity(bDisable ? FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)) : FSlateColor(FLinearColor::White));
			}
			else if (UBorder* Border = Cast<UBorder>(ChildWidget))
			{
				Border->SetContentColorAndOpacity(CardColor);
			}
		});
	}
}

FReply UWildCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsCardDisabled)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
		{
			if (!PC->IsInTacticalSkillCardSelection())
			{
				PC->CancelTileAction();
			}
		}

		bIsPressed = true;
		bIsDragging = false;
		MouseDownScreenPos = InMouseEvent.GetScreenSpacePosition();
		// 마우스를 이 위젯에 캡처하여 드래그나 클릭 종료를 정확히 감지
		return FReply::Handled().CaptureMouse(TakeWidget()); 
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWildCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsCardDisabled)
	{
		return FReply::Unhandled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
	{
		bIsPressed = false;

		if (bIsDragging)
		{
			bIsDragging = false;
			OnDragEnded.ExecuteIfBound(this, InMouseEvent.GetScreenSpacePosition());
		}
		else
		{
			// 드래그 거리 임계값을 넘지 않고 마우스를 뗀 경우 -> 클릭 이벤트 발생!
			OnClicked.ExecuteIfBound(this);
		}

		// 마우스 캡처 해제
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWildCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsCardDisabled)
	{
		return FReply::Unhandled();
	}

	if (bIsPressed && HasMouseCapture())
	{
		FVector2D CurrentPos = InMouseEvent.GetScreenSpacePosition();

		// 아직 드래그 상태가 아니지만 임계값을 넘은 경우 드래그 시작으로 전환
		if (!bIsDragging)
		{
			if (FVector2D::Distance(CurrentPos, MouseDownScreenPos) >= DragThreshold)
			{
				bIsDragging = true;
				OnDragStarted.ExecuteIfBound(this);
			}
		}

		if (bIsDragging)
		{
			OnDragMoved.ExecuteIfBound(this, CurrentPos);
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UWildCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (!bIsDragging && !bIsCardDisabled)
	{
		OnHovered.ExecuteIfBound(this);
	}
}

void UWildCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (!bIsDragging && !bIsCardDisabled)
	{
		OnUnhovered.ExecuteIfBound(this);
	}
}
