// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/WildCardWidget.h"
#include "WildCardViewModel.h"
#include "View/MVVMView.h"

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
}

FReply UWildCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsDragging = true;
		OnDragStarted.ExecuteIfBound(this);
		// 마우스를 이 위젯에 가두어 빠른 속도로 이동해도 놓치지 않게 함
		return FReply::Handled().CaptureMouse(TakeWidget()); 
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWildCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
	{
		bIsDragging = false;
		OnDragEnded.ExecuteIfBound(this, InMouseEvent.GetScreenSpacePosition());
		// 마우스 캡처 해제
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UWildCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDragging && HasMouseCapture())
	{
		OnDragMoved.ExecuteIfBound(this, InMouseEvent.GetScreenSpacePosition());
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UWildCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (!bIsDragging)
	{
		OnHovered.ExecuteIfBound(this);
	}
}

void UWildCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	if (!bIsDragging)
	{
		OnUnhovered.ExecuteIfBound(this);
	}
}
