// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class AWildLoginPlayerController;
class SEditableTextBox;
class STextBlock;

/**
 * LoginWidgetClass(UMG)가 아직 할당되지 않은 경우 검은 화면을 방지하고
 * 개발자/테스터가 즉시 회원가입 및 로그인을 테스트할 수 있도록 제공하는 폴백 Slate 위젯
 */
class SWildLoginFallbackWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildLoginFallbackWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AWildLoginPlayerController>, PlayerController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetStatusText(const FText& InText, const FLinearColor& InColor = FLinearColor::White);

private:
	FReply OnSignInClicked();
	FReply OnSignUpClicked();
	FReply OnQuickStartClicked();

	TWeakObjectPtr<AWildLoginPlayerController> PCWeak;
	TSharedPtr<SEditableTextBox> UsernameTextBox;
	TSharedPtr<SEditableTextBox> PasswordTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;
};
