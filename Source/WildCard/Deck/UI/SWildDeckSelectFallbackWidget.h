// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Deck/WildDeckSelectPlayerController.h"
#include "Deck/WildDeckSubsystem.h"
#include "Auth/WildAuthSubsystem.h"
#include "WildCardGameInstance.h"
#include "Engine/DataTable.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Types/SlateEnums.h"

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

/**
 * 덱 선택 개발용 Slate Fallback 위젯
 * WBP_DeckSelect(UMG)가 아직 지정되지 않았을 때 하스스톤 스타일의 덱 리스트 및 덱 선택 기능을 제공
 */
class SWildDeckSelectFallbackWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildDeckSelectFallbackWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AWildDeckSelectPlayerController>, PlayerController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		PCWeak = InArgs._PlayerController;

		ChildSlot
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.03f, 0.04f, 0.07f, 1.0f))
			.Padding(FMargin(24.f, 20.f))
			[
				SNew(SVerticalBox)

				// 1. 상단 헤더 바
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 16.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("W I L D   C A R D   -   덱 선택 (Deck Selection)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
						.ColorAndOpacity(FLinearColor(1.0f, 0.82f, 0.35f, 1.0f))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(UserTagText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor(0.75f, 0.8f, 0.9f, 1.0f))
					]
				]

				// 2. 메인 콘텐츠 (좌측: 덱 리스트 | 우측: 선택된 덱 상세 프리뷰)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(0.f, 0.f, 0.f, 16.f)
				[
					SNew(SHorizontalBox)

					// --- 좌측: 보유 덱 목록 ---
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 16.f, 0.f)
					[
						SNew(SBox)
						.WidthOverride(380.f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor(0.08f, 0.10f, 0.14f, 0.95f))
							.Padding(FMargin(16.f, 14.f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 10.f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("보유 덱 목록 (DECK LIST)")))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
									.ColorAndOpacity(FLinearColor(0.9f, 0.95f, 1.0f, 1.0f))
								]
								+ SVerticalBox::Slot()
								.FillHeight(1.0f)
								[
									SAssignNew(DeckListScrollBox, SScrollBox)
								]
							]
						]
					]

					// --- 우측: 덱 상세 정보 ---
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.08f, 0.10f, 0.14f, 0.95f))
						.Padding(FMargin(20.f, 16.f))
						[
							SAssignNew(DeckDetailContainer, SVerticalBox)
						]
					]
				]

				// 3. 하단 컨트롤 바 (뒤로가기, 상태 메시지, 대전 시작)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					// 뒤로가기 버튼
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(160.f)
						.HeightOverride(44.f)
						[
							SNew(SButton)
							.ButtonColorAndOpacity(FLinearColor(0.2f, 0.22f, 0.28f, 1.0f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked(this, &SWildDeckSelectFallbackWidget::OnBackButtonClicked)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("← 로그인 화면")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
								.ColorAndOpacity(FLinearColor::White)
							]
						]
					]

					// 상태 메시지
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(20.f, 0.f)
					[
						SAssignNew(StatusTextBlock, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
						.AutoWrapText(true)
					]

					// 대전 시작 버튼 (PLAY)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(220.f)
						.HeightOverride(48.f)
						[
							SAssignNew(PlayButton, SButton)
							.ButtonColorAndOpacity(FLinearColor(0.85f, 0.65f, 0.15f, 1.0f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.IsEnabled(this, &SWildDeckSelectFallbackWidget::IsPlayButtonEnabled)
							.OnClicked(this, &SWildDeckSelectFallbackWidget::OnPlayButtonClicked)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("대 전  시 작  ( P L A Y )")))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
								.ColorAndOpacity(FLinearColor::Black)
							]
						]
					]
				]
			]
		];

		RefreshUI();
	}

	void RefreshUI()
	{
		if (!PCWeak.IsValid()) return;
		AWildDeckSelectPlayerController* PC = PCWeak.Get();

		const FString Username = PC->GetActiveUsername();
		if (UserTagText.IsValid())
		{
			UserTagText->SetText(FText::FromString(FString::Printf(TEXT("접속 계정: %s"), *Username)));
		}

		UWildDeckSubsystem* DeckSub = nullptr;
		if (UGameInstance* GI = PC->GetGameInstance())
		{
			DeckSub = GI->GetSubsystem<UWildDeckSubsystem>();
		}

		if (!DeckSub)
		{
			SetStatusText(FText::FromString(TEXT("UWildDeckSubsystem을 찾을 수 없습니다.")), FLinearColor::Red);
			return;
		}

		TArray<FWildDeckData> Decks = DeckSub->GetUserDecks(Username);

		// 1. 덱 리스트 목록 갱신
		if (DeckListScrollBox.IsValid())
		{
			DeckListScrollBox->ClearChildren();

			for (int32 i = 0; i < Decks.Num(); ++i)
			{
				const FWildDeckData& Deck = Decks[i];
				const bool bIsSelected = DeckSub->HasSelectedDeck() && (DeckSub->GetSelectedDeck().DeckName == Deck.DeckName);
				const FLinearColor ClassColor = GetClassColor(Deck.DeckClass);

				DeckListScrollBox->AddSlot()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SBox)
					.HeightOverride(64.f)
					[
						SNew(SButton)
						.ButtonColorAndOpacity(bIsSelected ? FLinearColor(0.28f, 0.24f, 0.12f, 1.0f) : FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
						.OnClicked_Lambda([this, Deck]() -> FReply
						{
							if (PCWeak.IsValid())
							{
								PCWeak->SelectDeck(Deck);
							}
							RefreshUI();
							return FReply::Handled();
						})
						[
							SNew(SBorder)
							.BorderBackgroundColor(bIsSelected ? FLinearColor(1.0f, 0.8f, 0.2f, 1.0f) : FLinearColor::Transparent)
							.Padding(FMargin(bIsSelected ? 2.f : 0.f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(10.f, 0.f, 10.f, 0.f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("[%s]"), *GetClassDisplayName(Deck.DeckClass))))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
									.ColorAndOpacity(ClassColor)
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								.VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text(FText::FromString(Deck.DeckName))
										.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
										.ColorAndOpacity(bIsSelected ? FLinearColor(1.0f, 0.9f, 0.5f, 1.0f) : FLinearColor::White)
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.f, 2.f, 0.f, 0.f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(FString::Printf(TEXT("스킬: %s"), 
											Deck.SelectedTacticalSkillID.IsNone() ? TEXT("미장착") : *Deck.SelectedTacticalSkillID.ToString())))
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
										.ColorAndOpacity(FLinearColor(0.65f, 0.7f, 0.75f, 1.0f))
									]
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0.f, 0.f, 10.f, 0.f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("%d장"), Deck.CardIDs.Num())))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(Deck.IsValidDeck() ? FLinearColor(0.4f, 0.9f, 0.4f, 1.0f) : FLinearColor(0.9f, 0.4f, 0.4f, 1.0f))
								]
							]
						]
					]
				];
			}
		}

		// 2. 우측 상세 패널 갱신
		if (DeckDetailContainer.IsValid())
		{
			DeckDetailContainer->ClearChildren();

			if (!DeckSub->HasSelectedDeck())
			{
				DeckDetailContainer->AddSlot()
				.FillHeight(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("← 왼쪽 목록에서 덱을 선택해주세요.")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(FLinearColor(0.6f, 0.65f, 0.7f, 1.0f))
				];

				SetStatusText(FText::FromString(TEXT("대전에 사용할 덱을 선택해주세요.")), FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
			}
			else
			{
				const FWildDeckData& SelDeck = DeckSub->GetSelectedDeck();
				const FLinearColor ClassColor = GetClassColor(SelDeck.DeckClass);

				// 덱 기본 타이틀 및 직업
				DeckDetailContainer->AddSlot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(SelDeck.DeckName))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
							.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.4f, 1.0f))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.f, 4.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("직업: %s  |  카드 수: %d장"), 
								*GetClassDisplayName(SelDeck.DeckClass), SelDeck.CardIDs.Num())))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
							.ColorAndOpacity(ClassColor)
						]
					]
				];

				// 전술 스킬 정보
				DeckDetailContainer->AddSlot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
					.Padding(FMargin(10.f, 8.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("장착 전술 스킬: %s"), 
							SelDeck.SelectedTacticalSkillID.IsNone() ? TEXT("미장착 (기본)") : *SelDeck.SelectedTacticalSkillID.ToString())))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(FLinearColor(0.9f, 0.85f, 0.7f, 1.0f))
					]
				];

				// 마나 커브 계산 및 출력
				UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(PC->GetGameInstance());
				TArray<int32> ManaDistribution;
				ManaDistribution.Init(0, 8); // 0, 1, 2, 3, 4, 5, 6, 7+

				TMap<FName, int32> CardCounts;
				for (const FName& CardName : SelDeck.CardIDs)
				{
					CardCounts.FindOrAdd(CardName)++;

					int32 ManaCost = 1;
					if (GI && GI->CardDataTable)
					{
						if (FCardData* CardData = GI->CardDataTable->FindRow<FCardData>(CardName, TEXT("ManaCalc")))
						{
							ManaCost = CardData->BaseManaCost;
						}
					}
					const int32 ClampedCost = FMath::Clamp(ManaCost, 0, 7);
					ManaDistribution[ClampedCost]++;
				}

				FString ManaCurveText = TEXT("마나 커브: ");
				for (int32 c = 0; c <= 7; ++c)
				{
					if (c == 7)
					{
						ManaCurveText += FString::Printf(TEXT("[7+코: %d장]"), ManaDistribution[c]);
					}
					else
					{
						ManaCurveText += FString::Printf(TEXT("[%d코: %d장] "), c, ManaDistribution[c]);
					}
				}

				DeckDetailContainer->AddSlot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.06f, 0.08f, 0.12f, 1.0f))
					.Padding(FMargin(10.f, 6.f))
					[
						SNew(STextBlock)
						.Text(FText::FromString(ManaCurveText))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
						.ColorAndOpacity(FLinearColor(0.4f, 0.85f, 1.0f, 1.0f))
					]
				];

				// 카드 리스트 (스크롤 박스)
				TSharedPtr<SScrollBox> CardListScroll;
				DeckDetailContainer->AddSlot()
				.FillHeight(1.0f)
				[
					SAssignNew(CardListScroll, SScrollBox)
				];

				if (CardListScroll.IsValid())
				{
					for (const auto& Pair : CardCounts)
					{
						const FName& CardID = Pair.Key;
						const int32 Count = Pair.Value;

						FString CardDisplayName = CardID.ToString();
						int32 CardCost = 1;
						UDataTable* CardDT = (GI && GI->CardDataTable) ? GI->CardDataTable.Get() : nullptr;
						if (CardDT)
						{
							if (FCardData* CD = CardDT->FindRow<FCardData>(CardID, TEXT("DisplayList")))
							{
								if (!CD->CardName.IsEmpty())
								{
									CardDisplayName = CD->CardName;
								}
								CardCost = CD->BaseManaCost;
							}
						}

						CardListScroll->AddSlot()
						.Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(SBorder)
							.BorderBackgroundColor(FLinearColor(0.11f, 0.13f, 0.17f, 0.9f))
							.Padding(FMargin(8.f, 4.f))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.f, 0.f, 8.f, 0.f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("(%d)"), CardCost)))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(FLinearColor(0.3f, 0.7f, 1.0f, 1.0f))
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(CardDisplayName))
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
									.ColorAndOpacity(FLinearColor::White)
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(FText::FromString(FString::Printf(TEXT("x%d"), Count)))
									.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
									.ColorAndOpacity(Count >= 2 ? FLinearColor(1.0f, 0.8f, 0.3f, 1.0f) : FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
								]
							]
						];
					}
				}

				if (SelDeck.IsValidDeck())
				{
					SetStatusText(FText::Format(FText::FromString(TEXT("선택된 덱: '{0}' ({1}장) - 대전 시작 가능")),
						FText::FromString(SelDeck.DeckName), FText::AsNumber(SelDeck.CardIDs.Num())), FLinearColor(0.4f, 1.0f, 0.5f, 1.0f));
				}
				else
				{
					SetStatusText(FText::Format(FText::FromString(TEXT("주의: 선택된 덱 '{0}'은 카드 수({1}장)가 유효하지 않아 대전을 시작할 수 없습니다.")),
						FText::FromString(SelDeck.DeckName), FText::AsNumber(SelDeck.CardIDs.Num())), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
				}
			}
		}
	}

	void SetStatusText(const FText& InText, const FLinearColor& InColor)
	{
		if (StatusTextBlock.IsValid())
		{
			StatusTextBlock->SetText(InText);
			StatusTextBlock->SetColorAndOpacity(InColor);
		}
	}

private:
	FReply OnPlayButtonClicked()
	{
		if (PCWeak.IsValid())
		{
			PCWeak->RequestStartMatch();
		}
		return FReply::Handled();
	}

	FReply OnBackButtonClicked()
	{
		if (PCWeak.IsValid())
		{
			PCWeak->RequestReturnToLogin();
		}
		return FReply::Handled();
	}

	bool IsPlayButtonEnabled() const
	{
		if (!PCWeak.IsValid()) return false;
		if (UGameInstance* GI = PCWeak->GetGameInstance())
		{
			if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
			{
				return DeckSub->HasSelectedDeck() && DeckSub->GetSelectedDeck().IsValidDeck();
			}
		}
		return false;
	}

	TWeakObjectPtr<AWildDeckSelectPlayerController> PCWeak;
	TSharedPtr<STextBlock> UserTagText;
	TSharedPtr<SScrollBox> DeckListScrollBox;
	TSharedPtr<SVerticalBox> DeckDetailContainer;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SButton> PlayButton;
};
