// Fill out your copyright notice in the Description page of Project Settings.

#include "Auth/UI/SWildLoginFallbackWidget.h"
#include "Auth/WildLoginPlayerController.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Types/SlateEnums.h"

void SWildLoginFallbackWidget::Construct(const FArguments& InArgs)
{
	PCWeak = InArgs._PlayerController;

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.04f, 0.05f, 0.08f, 1.0f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(460.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.09f, 0.11f, 0.16f, 0.95f))
				.Padding(FMargin(32.f, 28.f))
				[
					SNew(SVerticalBox)

					// 타이틀
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("W I L D   C A R D")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
						.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.35f, 1.0f))
					]

					// 서브타이틀
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 24.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("로그인 / 회원가입 (Developer Fallback UI)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(FLinearColor(0.6f, 0.65f, 0.75f, 1.0f))
					]

					// 아이디 라벨
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("아이디 (Username)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
					]

					// 아이디 입력창
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 14.f)
					[
						SAssignNew(UsernameTextBox, SEditableTextBox)
						.HintText(FText::FromString(TEXT("아이디 입력 (최소 2자)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					]

					// 비밀번호 라벨
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("비밀번호 (Password)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.95f, 1.0f))
					]

					// 비밀번호 입력창
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 18.f)
					[
						SAssignNew(PasswordTextBox, SEditableTextBox)
						.HintText(FText::FromString(TEXT("비밀번호 입력 (최소 4자)")))
						.IsPassword(true)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					]

					// 버튼 영역 (로그인 / 회원가입)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 12.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ButtonColorAndOpacity(FLinearColor(0.18f, 0.45f, 0.85f, 1.0f))
							.OnClicked(this, &SWildLoginFallbackWidget::OnSignInClicked)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("로그인")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(FLinearColor::White)
								.Margin(FMargin(0.f, 6.f))
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(6.f, 0.f, 0.f, 0.f)
						[
							SNew(SButton)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.ButtonColorAndOpacity(FLinearColor(0.22f, 0.26f, 0.35f, 1.0f))
							.OnClicked(this, &SWildLoginFallbackWidget::OnSignUpClicked)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("회원가입")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 0.95f, 1.0f))
								.Margin(FMargin(0.f, 6.f))
							]
						]
					]

					// 빠른 시작 (로그인 건너뛰고 게스트로 바로 레벨 진입)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 14.f)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ButtonColorAndOpacity(FLinearColor(0.15f, 0.55f, 0.35f, 0.85f))
						.OnClicked(this, &SWildLoginFallbackWidget::OnQuickStartClicked)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("게스트 바로 시작 (Quick Start)")))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
							.ColorAndOpacity(FLinearColor(0.9f, 1.0f, 0.9f, 1.0f))
							.Margin(FMargin(0.f, 4.f))
						]
					]

					// 상태 메시지 텍스트
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SAssignNew(StatusTextBlock, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
						.ColorAndOpacity(FLinearColor(0.7f, 0.75f, 0.85f, 1.0f))
						.Text(FText::FromString(TEXT("아이디/비밀번호를 입력 후 로그인하세요.")))
					]
				]
			]
		]
	];
}

void SWildLoginFallbackWidget::SetStatusText(const FText& InText, const FLinearColor& InColor)
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InText);
		StatusTextBlock->SetColorAndOpacity(InColor);
	}
}

FReply SWildLoginFallbackWidget::OnSignInClicked()
{
	if (AWildLoginPlayerController* PC = PCWeak.Get())
	{
		const FString Username = UsernameTextBox.IsValid() ? UsernameTextBox->GetText().ToString() : FString();
		const FString Password = PasswordTextBox.IsValid() ? PasswordTextBox->GetText().ToString() : FString();
		PC->RequestSignInFromFallback(Username, Password);
	}
	return FReply::Handled();
}

FReply SWildLoginFallbackWidget::OnSignUpClicked()
{
	if (AWildLoginPlayerController* PC = PCWeak.Get())
	{
		const FString Username = UsernameTextBox.IsValid() ? UsernameTextBox->GetText().ToString() : FString();
		const FString Password = PasswordTextBox.IsValid() ? PasswordTextBox->GetText().ToString() : FString();
		PC->RequestSignUpFromFallback(Username, Password);
	}
	return FReply::Handled();
}

FReply SWildLoginFallbackWidget::OnQuickStartClicked()
{
	if (AWildLoginPlayerController* PC = PCWeak.Get())
	{
		PC->RequestQuickStartFromFallback();
	}
	return FReply::Handled();
}
