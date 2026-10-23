// Fill out your copyright notice in the Description page of Project Settings.

#include "SWildMatchmakingDialogWidget.h"
#include "WildDeckSelectPlayerController.h"
#include "WildMatchmakingSubsystem.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Types/SlateEnums.h"

static const TCHAR* RouletteMessages[] =
{
	TEXT("◎ 가장 만만한 상대 탐색 중..."),
	TEXT("◎ 전설의 대마법사 피하는 중..."),
	TEXT("◎ 비전 마나 증폭 및 주사위 굴리는 중..."),
	TEXT("◎ 덱 셔플 및 최적의 전장 물색 중..."),
	TEXT("◎ 호각의 실력을 가진 라이벌 탐색 중...")
};
static const int32 NumRouletteMessages = UE_ARRAY_COUNT(RouletteMessages);

static FString GetClassDisplayName(EWildPlayerClass InClass)
{
	switch (InClass)
	{
	case EWildPlayerClass::Warrior: return TEXT("전사");
	case EWildPlayerClass::Mage:    return TEXT("마법사");
	case EWildPlayerClass::Taoist:  return TEXT("도사");
	default:                        return TEXT("공용");
	}
}

static FLinearColor GetClassColor(EWildPlayerClass InClass)
{
	switch (InClass)
	{
	case EWildPlayerClass::Warrior: return FLinearColor(0.85f, 0.25f, 0.2f, 1.0f);
	case EWildPlayerClass::Mage:    return FLinearColor(0.2f, 0.55f, 0.95f, 1.0f);
	case EWildPlayerClass::Taoist:  return FLinearColor(0.2f, 0.85f, 0.45f, 1.0f);
	default:                        return FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);
	}
}

void SWildMatchmakingDialogWidget::Construct(const FArguments& InArgs)
{
	PCWeak = InArgs._PlayerController;

	ChildSlot
	[
		SAssignNew(BackdropBorder, SBorder)
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(500.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.09f, 0.11f, 0.16f, 0.98f))
				.Padding(FMargin(28.f, 24.f))
				[
					SAssignNew(ContentContainer, SVerticalBox)

					// 1. 헤더 타이틀
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(TitleText, STextBlock)
						.Text(FText::FromString(TEXT("W I L D   C A R D")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
						.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.35f, 1.0f))
					]

					// 2. 부제목 ("상대 찾는 중...")
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 16.f)
					[
						SAssignNew(SubTitleText, STextBlock)
						.Text(FText::FromString(TEXT("상  대    찾  는    중 . . .")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(FLinearColor::White)
					]

					// 3. 룰렛 회전판 연출 박스
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 18.f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.05f, 0.06f, 0.09f, 1.0f))
						.Padding(FMargin(16.f, 14.f))
						.HAlign(HAlign_Center)
						[
							SAssignNew(RouletteText, STextBlock)
							.Text(FText::FromString(RouletteMessages[0]))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
							.ColorAndOpacity(FLinearColor(0.4f, 0.85f, 1.0f, 1.0f))
						]
					]

					// 4. 타이머 (경과 시간 / 봇 타임아웃)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 0.f, 0.f, 20.f)
					[
						SAssignNew(TimerText, STextBlock)
						.Text(FText::FromString(TEXT("경과 시간: 00:00   /   봇 매칭까지: 10초")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor(0.75f, 0.8f, 0.85f, 1.0f))
					]

					// 5. 취소 버튼
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(180.f)
						.HeightOverride(42.f)
						[
							SAssignNew(CancelButton, SButton)
							.ButtonColorAndOpacity(FLinearColor(0.35f, 0.15f, 0.15f, 1.0f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked(this, &SWildMatchmakingDialogWidget::OnCancelClicked)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("취  소  ( C A N C E L )")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		]
	];
}

FReply SWildMatchmakingDialogWidget::OnCancelClicked()
{
	if (PCWeak.IsValid())
	{
		if (UWildMatchmakingSubsystem* MatchSub = PCWeak->GetGameInstance()->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->CancelMatchmaking();
		}
	}
	return FReply::Handled();
}

void SWildMatchmakingDialogWidget::OnMatchmakingStateChanged(EWildMatchmakingState NewState)
{
	if (NewState == EWildMatchmakingState::Searching)
	{
		if (SubTitleText.IsValid())
		{
			SubTitleText->SetText(FText::FromString(TEXT("상  대    찾  는    중 . . .")));
		}
	}
}

void SWildMatchmakingDialogWidget::OnTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot)
{
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;

	if (TimerText.IsValid())
	{
		TimerText->SetText(FText::FromString(
			FString::Printf(TEXT("경과 시간: %02d:%02d   /   봇 매칭까지: %d초"), Minutes, Seconds, RemainingSecondsUntilBot)));
	}

	UpdateWheelAnimationText();
}

void SWildMatchmakingDialogWidget::UpdateWheelAnimationText()
{
	RouletteIndex = (RouletteIndex + 1) % NumRouletteMessages;
	if (RouletteText.IsValid())
	{
		RouletteText->SetText(FText::FromString(RouletteMessages[RouletteIndex]));
	}
}

void SWildMatchmakingDialogWidget::OnMatchFound(const FWildMatchResult& MatchResult)
{
	if (!ContentContainer.IsValid()) return;

	ContentContainer->ClearChildren();

	const FLinearColor MyClassColor = GetClassColor(MatchResult.MyDeckData.DeckClass);
	const FLinearColor OpponentClassColor = GetClassColor(MatchResult.Opponent.PlayerClass);

	ContentContainer->AddSlot()
	.AutoHeight()
	.HAlign(HAlign_Center)
	.Padding(0.f, 0.f, 0.f, 10.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("★  상 대 를   찾 았 습 니 다 !  ★")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
		.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.2f, 1.0f))
	];

	// VS 대전 구도
	ContentContainer->AddSlot()
	.AutoHeight()
	.Padding(0.f, 10.f, 0.f, 16.f)
	[
		SNew(SHorizontalBox)

		// 내 프로필
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("[ 나 ]")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MatchResult.MyDeckData.DeckName))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(GetClassDisplayName(MatchResult.MyDeckData.DeckClass)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(MyClassColor)
			]
		]

		// VS 배너
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(16.f, 0.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("V S")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
			.ColorAndOpacity(FLinearColor(1.0f, 0.4f, 0.2f, 1.0f))
		]

		// 상대 프로필
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MatchResult.Opponent.bIsBot ? TEXT("[ 인공지능 상대 ]") : TEXT("[ 도전자 ]")))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.ColorAndOpacity(MatchResult.Opponent.bIsBot ? FLinearColor(0.4f, 0.8f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.8f, 0.4f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(MatchResult.Opponent.PlayerName))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FLinearColor::White)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(GetClassDisplayName(MatchResult.Opponent.PlayerClass)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(OpponentClassColor)
			]
		]
	];

	// 하단 진행 상태 문구
	ContentContainer->AddSlot()
	.AutoHeight()
	.HAlign(HAlign_Center)
	.Padding(0.f, 10.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("전장으로 이동하는 중입니다... (잠시만 기다려주세요)")))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
		.ColorAndOpacity(FLinearColor(0.4f, 1.0f, 0.6f, 1.0f))
	];
}
