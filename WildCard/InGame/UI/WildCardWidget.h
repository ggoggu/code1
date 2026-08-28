// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WildCardWidget.generated.h"

class UWildCardViewModel;

DECLARE_DELEGATE_OneParam(FOnCardHoverStateChanged, class UWildCardWidget* /*Widget*/);
DECLARE_DELEGATE_TwoParams(FOnCardDragStateChanged, class UWildCardWidget* /*Widget*/, FVector2D /*ScreenPos*/);

UCLASS()
class WILDCARD_API UWildCardWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 이 위젯이 화면에 표시하기 위해 참조하는 데이터(뷰모델)
	UPROPERTY(BlueprintReadOnly, Category = "Card|ViewModel")
	UWildCardViewModel* CardViewModel;

public:
	// 핸드 위젯(WildHandWidget)이 이 카드를 생성할 때 데이터를 주입하는 함수
	UFUNCTION(BlueprintCallable, Category = "Card")
	void SetViewModel(UWildCardViewModel* InViewModel);

	UWildCardViewModel* GetViewModel() const { return CardViewModel; }
	
	FOnCardHoverStateChanged OnHovered;
	FOnCardHoverStateChanged OnUnhovered;
	FOnCardHoverStateChanged OnDragStarted;
	FOnCardDragStateChanged OnDragMoved;
	FOnCardDragStateChanged OnDragEnded;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
private:
	bool bIsDragging = false;
	
};
