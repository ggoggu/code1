// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/UI/Widget/WildCombatLogWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "InGame/Battle/WildCombatLogComponent.h"
#include "InGame/UI/ViewModel/WildHUDViewModel.h"
#include "InGame/UI/ViewModel/WildCombatLogEntryViewModel.h"
#include "View/MVVMView.h"

void UWildCombatLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!LogScrollBox)
	{
		BuildDynamicUIFallback();
	}

	if (Button_Close)
	{
		Button_Close->OnClicked.RemoveDynamic(this, &UWildCombatLogWidget::OnCloseButtonClicked);
		Button_Close->OnClicked.AddDynamic(this, &UWildCombatLogWidget::OnCloseButtonClicked);
	}

	if (AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr)
	{
		if (UWildCombatLogComponent* LogComp = GS->GetCombatLogComponent())
		{
			LogComp->OnCombatLogAdded.RemoveDynamic(this, &UWildCombatLogWidget::HandleCombatLogAdded);
			LogComp->OnCombatLogAdded.AddDynamic(this, &UWildCombatLogWidget::HandleCombatLogAdded);
		}
	}

	RefreshLogs();
}

void UWildCombatLogWidget::InitializeCombatLogWidget(UWildHUDViewModel* InViewModel)
{
	if (!InViewModel) return;

	HUDViewModel = InViewModel;

	// MVVM View 익스텐션에 뷰모델 등록
	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		View->SetViewModel(FName("WildHUDViewModel"), InViewModel);
	}

	RefreshLogs();
}

void UWildCombatLogWidget::OnCloseButtonClicked()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (HUDViewModel)
	{
		HUDViewModel->SetCombatLogOpen(false);
	}
}

void UWildCombatLogWidget::ToggleVisibility()
{
	bool bNewOpen = (GetVisibility() != ESlateVisibility::Visible);

	if (bNewOpen)
	{
		SetVisibility(ESlateVisibility::Visible);
		RefreshLogs();
		if (LogScrollBox)
		{
			LogScrollBox->ScrollToEnd();
		}
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HUDViewModel)
	{
		HUDViewModel->SetCombatLogOpen(bNewOpen);
	}
}

void UWildCombatLogWidget::RefreshLogs()
{
	if (!LogScrollBox) return;

	LogScrollBox->ClearChildren();

	if (HUDViewModel)
	{
		HUDViewModel->ClearCombatLogEntryViewModels();
	}

	if (AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr)
	{
		if (UWildCombatLogComponent* LogComp = GS->GetCombatLogComponent())
		{
			const TArray<FWildCombatLogEntry>& History = LogComp->GetLogHistory();
			for (const FWildCombatLogEntry& Entry : History)
			{
				AppendLogEntryUI(Entry);
			}
		}
	}

	LogScrollBox->ScrollToEnd();
}

void UWildCombatLogWidget::HandleCombatLogAdded(const FWildCombatLogEntry& NewEntry)
{
	AppendLogEntryUI(NewEntry);
}

void UWildCombatLogWidget::AppendLogEntryUI(const FWildCombatLogEntry& Entry)
{
	int32 LocalID = GetLocalPlayerID();
	int32 FirstID = GetFirstPlayerID();

	// 뷰모델 동기화 (MVVM 바인딩용)
	if (HUDViewModel)
	{
		UWildCombatLogEntryViewModel* EntryVM = NewObject<UWildCombatLogEntryViewModel>(this);
		EntryVM->InitializeFromEntry(Entry, LocalID, FirstID);
		HUDViewModel->AddCombatLogEntryViewModel(EntryVM);
	}

	if (!LogScrollBox || !WidgetTree) return;

	FString Title;
	TArray<FString> Details;
	UWildCombatLogComponent::FormatLogEntry(Entry, LocalID, FirstID, Title, Details);

	// 각 로그 엔트리를 감쌀 테두리 박스
	UBorder* EntryBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!EntryBorder) return;

	EntryBorder->SetBrushColor(FLinearColor(0.12f, 0.12f, 0.16f, 0.75f));
	EntryBorder->SetPadding(FMargin(8.0f, 6.0f, 8.0f, 6.0f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	EntryBorder->AddChild(VBox);

	// 타이틀 텍스트
	UTextBlock* TitleBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (TitleBlock)
	{
		TitleBlock->SetText(FText::FromString(Title));
		FSlateFontInfo FontInfo = TitleBlock->GetFont();
		FontInfo.Size = 13;
		TitleBlock->SetFont(FontInfo);
		TitleBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.45f, 1.0f))); // Gold Tint
		VBox->AddChildToVerticalBox(TitleBlock);
	}

	// 상세 줄들
	for (const FString& Line : Details)
	{
		UTextBlock* LineBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		if (LineBlock)
		{
			LineBlock->SetText(FText::FromString(Line));
			FSlateFontInfo FontInfo = LineBlock->GetFont();
			FontInfo.Size = 12;
			LineBlock->SetFont(FontInfo);

			if (Line.Contains(TEXT("아군")) || Line.StartsWith(TEXT("=아군")))
			{
				LineBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.82f, 1.0f, 1.0f))); // Cyan
			}
			else if (Line.Contains(TEXT("적")) || Line.StartsWith(TEXT("=적")))
			{
				LineBlock->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.45f, 0.45f, 1.0f))); // Coral Red
			}
			else
			{
				LineBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.88f, 1.0f))); // Light Gray
			}

			VBox->AddChildToVerticalBox(LineBlock);
		}
	}

	LogScrollBox->AddChild(EntryBorder);
	LogScrollBox->ScrollToEnd();
}

void UWildCombatLogWidget::BuildDynamicUIFallback()
{
	if (!WidgetTree) return;

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LogRootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.07f, 0.92f)); // Dark theme
	RootBorder->SetPadding(FMargin(10.0f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* MainVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVBox"));
	RootBorder->AddChild(MainVBox);

	// 1. 헤더 바 (가로 배치: 타이틀 + 닫기 버튼)
	UHorizontalBox* HeaderBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderBar"));
	UVerticalBoxSlot* HeaderSlot = MainVBox->AddChildToVerticalBox(HeaderBar);
	if (HeaderSlot)
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	Text_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Title"));
	Text_Title->SetText(FText::FromString(TEXT("전투 기록 (Battle Log)")));
	FSlateFontInfo TitleFont = Text_Title->GetFont();
	TitleFont.Size = 15;
	Text_Title->SetFont(TitleFont);
	Text_Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.9f, 0.6f, 1.0f)));

	UHorizontalBoxSlot* TitleSlot = HeaderBar->AddChildToHorizontalBox(Text_Title);
	if (TitleSlot)
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
	}

	Button_Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Close"));
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseText"));
	CloseText->SetText(FText::FromString(TEXT("닫기 X")));
	FSlateFontInfo CloseFont = CloseText->GetFont();
	CloseFont.Size = 12;
	CloseText->SetFont(CloseFont);
	Button_Close->AddChild(CloseText);

	UHorizontalBoxSlot* CloseSlot = HeaderBar->AddChildToHorizontalBox(Button_Close);
	if (CloseSlot)
	{
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// 2. 스크롤 박스
	LogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LogScrollBox"));
	UVerticalBoxSlot* ScrollSlot = MainVBox->AddChildToVerticalBox(LogScrollBox);
	if (ScrollSlot)
	{
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
}

int32 UWildCombatLogWidget::GetLocalPlayerID() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
		{
			return PS->GetPlayerId();
		}
	}
	return -1;
}

int32 UWildCombatLogWidget::GetFirstPlayerID() const
{
	if (AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr)
	{
		return GS->GetCurrentPriorityPlayerID();
	}
	return -1;
}
