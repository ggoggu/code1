// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/UI/Widget/WildNoticeWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"

UWildNoticeWidget::UWildNoticeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultDisplayDuration = 2.0f;
	FadeDuration = 0.25f;
	ElapsedTime = 0.0f;
	TotalDisplayTime = 2.0f;
	bIsDismissing = false;
	bIsShowing = false;
	NoticeType = EWildTimingNoticeType::None;
}

void UWildNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BuildDynamicUIIfNeeded();
}

void UWildNoticeWidget::BuildDynamicUIIfNeeded()
{
	// 디자이너가 UMG 에디터에서 Text_NoticeMessage를 배치하지 않았거나 C++ 클래스 그대로 생성된 경우,
	// 깔끔한 배너 형태의 기본 UI를 동적으로 생성합니다.
	if (Text_NoticeMessage)
	{
		return;
	}

	// 블루프린트로 제작된 위젯(WBP_PrivatePhase 등)의 경우 이미 RootWidget이 존재합니다.
	// 절대로 WidgetTree->RootWidget을 덮어쓰면 안 됩니다 (기존 위젯 트리가 언루트되어 GC로 파괴되고 Slate 렌더링 시 UAF 크래시 발생).
	if (WidgetTree && WidgetTree->RootWidget)
	{
		WidgetTree->ForEachWidget([this](UWidget* ChildWidget)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(ChildWidget))
			{
				if (!Text_NoticeTitle && TextBlock->GetName().Contains(TEXT("Title")))
				{
					Text_NoticeTitle = TextBlock;
				}
				else if (!Text_NoticeMessage)
				{
					Text_NoticeMessage = TextBlock;
				}
			}
		});
		return;
	}

	UCanvasPanel* RootCanvas = nullptr;
	if (WidgetTree)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (!RootCanvas)
	{
		RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	}

	if (!RootCanvas && WidgetTree)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Dynamic_RootCanvas"));
		if (RootCanvas)
		{
			WidgetTree->RootWidget = RootCanvas;
		}
	}

	if (RootCanvas && WidgetTree)
	{
		Border_Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dynamic_Border"));
		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Dynamic_VBox"));
		Text_NoticeTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Dynamic_TextTitle"));
		Text_NoticeMessage = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Dynamic_TextMessage"));

		if (Border_Background && VBox && Text_NoticeTitle && Text_NoticeMessage)
		{
			// 배경 스타일링 (반투명 어두운 그라디언트 느낌)
			Border_Background->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.88f));
			Border_Background->SetPadding(FMargin(40.0f, 16.0f, 40.0f, 18.0f));

			// 타이틀 텍스트 스타일링
			FSlateFontInfo TitleFont = Text_NoticeTitle->GetFont();
			TitleFont.Size = 13;
			Text_NoticeTitle->SetFont(TitleFont);
			Text_NoticeTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 0.8f)));
			Text_NoticeTitle->SetJustification(ETextJustify::Center);

			// 메인 메시지 텍스트 스타일링
			FSlateFontInfo MessageFont = Text_NoticeMessage->GetFont();
			MessageFont.Size = 28;
			Text_NoticeMessage->SetFont(MessageFont);
			Text_NoticeMessage->SetJustification(ETextJustify::Center);

			// 배치
			if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Text_NoticeTitle))
			{
				TitleSlot->SetHorizontalAlignment(HAlign_Center);
				TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
			}

			if (UVerticalBoxSlot* MsgSlot = VBox->AddChildToVerticalBox(Text_NoticeMessage))
			{
				MsgSlot->SetHorizontalAlignment(HAlign_Center);
			}

			Border_Background->AddChild(VBox);

			if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Border_Background))
			{
				CanvasSlot->SetAnchors(FAnchors(0.5f, 0.30f, 0.5f, 0.30f));
				CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				CanvasSlot->SetAutoSize(true);
			}
		}
	}
}

void UWildNoticeWidget::ShowNotice(EWildTimingNoticeType InType, const FText& InMessage, float InDuration)
{
	NoticeType = InType;
	TotalDisplayTime = (InDuration > 0.0f) ? InDuration : DefaultDisplayDuration;
	ElapsedTime = 0.0f;
	bIsDismissing = false;
	bIsShowing = true;

	BuildDynamicUIIfNeeded();

	// 타이틀 및 메시지 텍스트 결정
	FText FinalTitle = GetDefaultTitleForNoticeType(InType);
	FText FinalMessage = !InMessage.IsEmpty() ? InMessage : GetDefaultMessageForNoticeType(InType);
	NoticeMessage = FinalMessage;

	FLinearColor AccentColor = GetColorForNoticeType(InType);

	if (Text_NoticeTitle)
	{
		Text_NoticeTitle->SetText(FinalTitle);
	}

	if (Text_NoticeMessage)
	{
		Text_NoticeMessage->SetText(FinalMessage);
		Text_NoticeMessage->SetColorAndOpacity(FSlateColor(AccentColor));
	}

	// 초기 연출 상태 설정 (페이드 인 시작)
	SetRenderOpacity(0.0f);
	SetRenderScale(FVector2D(1.15f, 1.15f));

	if (Anim_Intro)
	{
		PlayAnimation(Anim_Intro);
	}

	// 블루프린트 연출 훅 호출
	BP_OnNoticeShown(InType, FinalMessage, TotalDisplayTime);
}

void UWildNoticeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsShowing)
	{
		return;
	}

	ElapsedTime += InDeltaTime;

	// 페이드 인 / 아웃 및 펀치 스케일 효과
	if (ElapsedTime < FadeDuration)
	{
		// 페이드 인 (0.0 -> 1.0) & 스케일 (1.15 -> 1.0)
		float Alpha = FMath::Clamp(ElapsedTime / FadeDuration, 0.0f, 1.0f);
		SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f));
		float Scale = FMath::InterpEaseOut(1.15f, 1.0f, Alpha, 2.0f);
		SetRenderScale(FVector2D(Scale, Scale));
	}
	else if (ElapsedTime < (TotalDisplayTime - FadeDuration))
	{
		// 유지 구간
		SetRenderOpacity(1.0f);
		SetRenderScale(FVector2D(1.0f, 1.0f));
	}
	else if (ElapsedTime < TotalDisplayTime)
	{
		// 페이드 아웃 (1.0 -> 0.0) & 스케일 (1.0 -> 0.95)
		float OutroTime = ElapsedTime - (TotalDisplayTime - FadeDuration);
		float Alpha = FMath::Clamp(OutroTime / FadeDuration, 0.0f, 1.0f);
		SetRenderOpacity(FMath::InterpEaseIn(1.0f, 0.0f, Alpha, 2.0f));
		float Scale = FMath::InterpEaseIn(1.0f, 0.95f, Alpha, 2.0f);
		SetRenderScale(FVector2D(Scale, Scale));
	}
	else
	{
		// 시간 만료 -> 닫기 실행
		bIsShowing = false;
		DismissNotice();
	}
}

void UWildNoticeWidget::DismissNotice()
{
	if (bIsDismissing)
	{
		return;
	}
	bIsDismissing = true;
	bIsShowing = false;

	if (Anim_Outro)
	{
		PlayAnimation(Anim_Outro);
		float OutroDuration = Anim_Outro->GetEndTime();
		if (OutroDuration > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				DismissTimerHandle,
				this,
				&UWildNoticeWidget::FinishDismiss,
				OutroDuration,
				false
			);
			return;
		}
	}

	FinishDismiss();
}

void UWildNoticeWidget::FinishDismiss()
{
	BP_OnNoticeDismissed();
	OnNoticeDismissed.Broadcast();
	if (IsInViewport() || GetParent() != nullptr)
	{
		RemoveFromParent();
	}
}

FLinearColor UWildNoticeWidget::GetColorForNoticeType(EWildTimingNoticeType InType) const
{
	switch (InType)
	{
	case EWildTimingNoticeType::TurnStart:
		return FLinearColor(1.0f, 0.84f, 0.05f, 1.0f); // 골드

	case EWildTimingNoticeType::ActionPhase:
		return FLinearColor(0.25f, 0.85f, 1.0f, 1.0f); // 사이언/하늘색

	case EWildTimingNoticeType::ChainStart:
		return FLinearColor(1.0f, 0.45f, 0.1f, 1.0f);  // 강렬한 주황

	case EWildTimingNoticeType::ChainResolution:
		return FLinearColor(0.85f, 0.35f, 1.0f, 1.0f); // 신비로운 보라

	case EWildTimingNoticeType::TurnEnd:
		return FLinearColor(0.75f, 0.75f, 0.75f, 1.0f); // 실버 그레이

	case EWildTimingNoticeType::Gameset:
		return FLinearColor(0.3f, 0.9f, 0.6f, 1.0f);   // 에메랄드

	case EWildTimingNoticeType::Mulligan:
		return FLinearColor(0.4f, 0.75f, 1.0f, 1.0f);  // 라이트 블루

	case EWildTimingNoticeType::Victory:
		return FLinearColor(1.0f, 0.95f, 0.2f, 1.0f);  // 눈부신 황금

	case EWildTimingNoticeType::Defeat:
		return FLinearColor(0.95f, 0.2f, 0.2f, 1.0f);  // 크림슨 레드

	case EWildTimingNoticeType::MainActionReveal:
		return FLinearColor(0.2f, 0.8f, 1.0f, 1.0f);   // 사이언/청록

	case EWildTimingNoticeType::MainActionResolution:
		return FLinearColor(1.0f, 0.45f, 0.1f, 1.0f);  // 강렬한 주황/골드

	case EWildTimingNoticeType::Custom:
	default:
		return FLinearColor::White;
	}
}

FText UWildNoticeWidget::GetDefaultTitleForNoticeType(EWildTimingNoticeType InType) const
{
	switch (InType)
	{
	case EWildTimingNoticeType::TurnStart:
	case EWildTimingNoticeType::TurnEnd:
		return FText::FromString(TEXT("TURN"));

	case EWildTimingNoticeType::ActionPhase:
		return FText::FromString(TEXT("PHASE"));

	case EWildTimingNoticeType::ChainStart:
	case EWildTimingNoticeType::ChainResolution:
		return FText::FromString(TEXT("CHAIN"));

	case EWildTimingNoticeType::Gameset:
	case EWildTimingNoticeType::Mulligan:
		return FText::FromString(TEXT("PREPARATION"));

	case EWildTimingNoticeType::Victory:
	case EWildTimingNoticeType::Defeat:
		return FText::FromString(TEXT("MATCH RESULT"));

	case EWildTimingNoticeType::MainActionReveal:
	case EWildTimingNoticeType::MainActionResolution:
		return FText::FromString(TEXT("MAIN ACTION"));

	case EWildTimingNoticeType::Custom:
	default:
		return FText::FromString(TEXT("NOTICE"));
	}
}

FText UWildNoticeWidget::GetDefaultMessageForNoticeType(EWildTimingNoticeType InType) const
{
	switch (InType)
	{
	case EWildTimingNoticeType::TurnStart:
		return FText::FromString(TEXT("턴 시작"));

	case EWildTimingNoticeType::ActionPhase:
		return FText::FromString(TEXT("행동 결정 단계"));

	case EWildTimingNoticeType::ChainStart:
		return FText::FromString(TEXT("체인 시작"));

	case EWildTimingNoticeType::ChainResolution:
		return FText::FromString(TEXT("체인 처리"));

	case EWildTimingNoticeType::TurnEnd:
		return FText::FromString(TEXT("턴 종료"));

	case EWildTimingNoticeType::Gameset:
		return FText::FromString(TEXT("게임 세팅 단계"));

	case EWildTimingNoticeType::Mulligan:
		return FText::FromString(TEXT("멀리건 단계"));

	case EWildTimingNoticeType::Victory:
		return FText::FromString(TEXT("승리!"));

	case EWildTimingNoticeType::Defeat:
		return FText::FromString(TEXT("패배..."));

	case EWildTimingNoticeType::MainActionReveal:
		return FText::FromString(TEXT("메인 행동 공개"));

	case EWildTimingNoticeType::MainActionResolution:
		return FText::FromString(TEXT("메인 행동 처리"));

	case EWildTimingNoticeType::Custom:
	default:
		return FText::FromString(TEXT("알림"));
	}
}
