// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/Widget/WildHUDWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "WildHandWidget.h"
#include "WildCardWidget.h"
#include "WildHUDViewModel.h"
#include "WildDragDropOperation.h"
#include "WildCardViewModel.h"
#include "InGameController.h"
#include "InGamePlayerState.h"
#include "InGameState.h"
#include "WildEnemyHandWidget.h"
#include "SelectActionWidget.h"
#include "InGame/Effect/TileSelectComponent.h"
#include "View/MVVMView.h"
#include "WildNoticeWidget.h"
#include "Components/PanelWidget.h"
#include "InGame/UI/Widget/WildCombatLogWidget.h"
#include "InGame/UI/Widget/WildVisualEffectWidget.h"

void UWildHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Button_SkipAction)
	{
		UCanvasPanel* RootCanvas = nullptr;
		if (WidgetTree)
		{
			RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
		}
		if (!RootCanvas)
		{
			RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
		}

		if (RootCanvas && WidgetTree)
		{
			Button_SkipAction = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_SkipAction_Dynamic"));
			Text_SkipAction = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_SkipAction_Dynamic"));

			if (Button_SkipAction && Text_SkipAction)
			{
				Text_SkipAction->SetText(FText::FromString(TEXT("스킵 / 패스")));
				Text_SkipAction->SetJustification(ETextJustify::Center);

				FSlateFontInfo FontInfo = Text_SkipAction->GetFont();
				FontInfo.Size = 16;
				Text_SkipAction->SetFont(FontInfo);

				Button_SkipAction->AddChild(Text_SkipAction);

				if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Button_SkipAction))
				{
					CanvasSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
					CanvasSlot->SetAlignment(FVector2D(1.0f, 1.0f));
					CanvasSlot->SetPosition(FVector2D(-40.0f, -120.0f));
					CanvasSlot->SetSize(FVector2D(160.0f, 50.0f));
					CanvasSlot->SetAutoSize(false);
				}
			}
		}
	}

	if (Button_SkipAction)
	{
		Button_SkipAction->OnClicked.RemoveDynamic(this, &UWildHUDWidget::OnSkipButtonClicked);
		Button_SkipAction->OnClicked.AddDynamic(this, &UWildHUDWidget::OnSkipButtonClicked);
	}

	// =========================================================================
	// 전투 로그 토글 버튼 (HUD 우측 상단) 동적 생성 및 바인딩
	// =========================================================================
	UCanvasPanel* RootCanvas = nullptr;
	if (WidgetTree)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	if (!RootCanvas)
	{
		RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	}

	if (!Button_LogToggle && RootCanvas && WidgetTree)
	{
		Button_LogToggle = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_LogToggle_Dynamic"));
		Text_LogToggle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_LogToggle_Dynamic"));

		if (Button_LogToggle && Text_LogToggle)
		{
			Text_LogToggle->SetText(FText::FromString(TEXT("로그")));
			Text_LogToggle->SetJustification(ETextJustify::Center);

			FSlateFontInfo LogFont = Text_LogToggle->GetFont();
			LogFont.Size = 14;
			Text_LogToggle->SetFont(LogFont);

			Button_LogToggle->AddChild(Text_LogToggle);

			if (UCanvasPanelSlot* LogSlot = RootCanvas->AddChildToCanvas(Button_LogToggle))
			{
				LogSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f)); // Top Right
				LogSlot->SetAlignment(FVector2D(1.0f, 0.0f));
				LogSlot->SetPosition(FVector2D(-30.0f, 25.0f));
				LogSlot->SetSize(FVector2D(80.0f, 36.0f));
				LogSlot->SetAutoSize(false);
			}
		}
	}

	if (Button_LogToggle)
	{
		Button_LogToggle->OnClicked.RemoveDynamic(this, &UWildHUDWidget::OnLogButtonClicked);
		Button_LogToggle->OnClicked.AddDynamic(this, &UWildHUDWidget::OnLogButtonClicked);
	}

	// 전투 로그 패널 동적 생성 (기본 Collapsed)
	if (!BP_CombatLogWidget && RootCanvas && WidgetTree)
	{
		TSubclassOf<UWildCombatLogWidget> WidgetClassToUse = CombatLogWidgetClass ? CombatLogWidgetClass : TSubclassOf<UWildCombatLogWidget>(UWildCombatLogWidget::StaticClass());
		BP_CombatLogWidget = WidgetTree->ConstructWidget<UWildCombatLogWidget>(WidgetClassToUse, TEXT("BP_CombatLogWidget_Dynamic"));
		if (BP_CombatLogWidget)
		{
			if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(BP_CombatLogWidget))
			{
				PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f)); // Top Right Dock
				PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
				PanelSlot->SetPosition(FVector2D(-25.0f, 70.0f));
				PanelSlot->SetSize(FVector2D(380.0f, 520.0f));
				PanelSlot->SetAutoSize(false);
			}
			BP_CombatLogWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// =========================================================================
	// 범용 카드 선택 모달 (서고 탐색 등) 반투명 암전 오버레이, 안내 텍스트, 확인 버튼
	// =========================================================================
	if (Border_CardSelectionDim && !Border_SearchLibraryDim) Border_SearchLibraryDim = Border_CardSelectionDim;
	if (Text_CardSelectionPrompt && !Text_SearchLibraryPrompt) Text_SearchLibraryPrompt = Text_CardSelectionPrompt;
	if (Button_CardSelectionConfirm && !Button_SearchLibraryConfirm) Button_SearchLibraryConfirm = Button_CardSelectionConfirm;
	if (Text_CardSelectionConfirm && !Text_SearchLibraryConfirm) Text_SearchLibraryConfirm = Text_CardSelectionConfirm;

	if (!Border_SearchLibraryDim && RootCanvas && WidgetTree)
	{
		Border_SearchLibraryDim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_SearchLibraryDim_Dynamic"));
		if (Border_SearchLibraryDim)
		{
			Border_SearchLibraryDim->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
			if (UCanvasPanelSlot* DimSlot = RootCanvas->AddChildToCanvas(Border_SearchLibraryDim))
			{
				DimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				DimSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
				DimSlot->SetZOrder(50);
			}
			Border_SearchLibraryDim->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!Text_SearchLibraryPrompt && RootCanvas && WidgetTree)
	{
		Text_SearchLibraryPrompt = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_SearchLibraryPrompt_Dynamic"));
		if (Text_SearchLibraryPrompt)
		{
			Text_SearchLibraryPrompt->SetText(FText::FromString(TEXT("패에서 카드 한장을 선택하세요")));
			Text_SearchLibraryPrompt->SetJustification(ETextJustify::Center);
			FSlateFontInfo PromptFont = Text_SearchLibraryPrompt->GetFont();
			PromptFont.Size = 24;
			PromptFont.OutlineSettings.OutlineSize = 2;
			PromptFont.OutlineSettings.OutlineColor = FLinearColor::Black;
			Text_SearchLibraryPrompt->SetFont(PromptFont);
			Text_SearchLibraryPrompt->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.95f, 0.8f, 1.0f)));
			Text_SearchLibraryPrompt->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
			Text_SearchLibraryPrompt->SetShadowOffset(FVector2D(2.0f, 2.0f));

			if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(Text_SearchLibraryPrompt))
			{
				PromptSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				PromptSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				PromptSlot->SetPosition(FVector2D(0.0f, -220.0f));
				PromptSlot->SetAutoSize(true);
				PromptSlot->SetZOrder(260);
			}
			Text_SearchLibraryPrompt->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (!Button_SearchLibraryConfirm && RootCanvas && WidgetTree)
	{
		Button_SearchLibraryConfirm = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_SearchLibraryConfirm_Dynamic"));
		Text_SearchLibraryConfirm = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_SearchLibraryConfirm_Dynamic"));

		if (Button_SearchLibraryConfirm && Text_SearchLibraryConfirm)
		{
			Text_SearchLibraryConfirm->SetText(FText::FromString(TEXT("확인")));
			Text_SearchLibraryConfirm->SetJustification(ETextJustify::Center);
			FSlateFontInfo BtnFont = Text_SearchLibraryConfirm->GetFont();
			BtnFont.Size = 18;
			Text_SearchLibraryConfirm->SetFont(BtnFont);

			Button_SearchLibraryConfirm->AddChild(Text_SearchLibraryConfirm);

			if (UCanvasPanelSlot* BtnSlot = RootCanvas->AddChildToCanvas(Button_SearchLibraryConfirm))
			{
				BtnSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				BtnSlot->SetAlignment(FVector2D(0.5f, 0.0f));
				BtnSlot->SetPosition(FVector2D(0.0f, 210.0f));
				BtnSlot->SetSize(FVector2D(140.0f, 48.0f));
				BtnSlot->SetAutoSize(false);
				BtnSlot->SetZOrder(260);
			}
			Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->OnClicked.RemoveDynamic(this, &UWildHUDWidget::OnCardSelectionConfirmClicked);
		Button_SearchLibraryConfirm->OnClicked.AddDynamic(this, &UWildHUDWidget::OnCardSelectionConfirmClicked);
	}

	// =========================================================================
	// PlayerUnit 전용 액션 메뉴 위젯 (WBP_SelectActionWidget) 동적 생성 및 바인딩
	// =========================================================================
	if (!BP_PlayerSelectActionWidget)
	{
		if (PlayerSelectActionWidgetClass)
		{
			BP_PlayerSelectActionWidget = CreateWidget<USelectActionWidget>(this, PlayerSelectActionWidgetClass);
			if (BP_PlayerSelectActionWidget)
			{
				if (RootCanvas)
				{
					if (UCanvasPanelSlot* ActionSlot = RootCanvas->AddChildToCanvas(BP_PlayerSelectActionWidget))
					{
						ActionSlot->SetAutoSize(true);
					}
				}
				BP_PlayerSelectActionWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[WildHUD] PlayerSelectActionWidgetClass가 설정되지 않았습니다. WBP_HUD 위젯 블루프린트 기본값에서 클래스를 지정해주세요."));
		}
	}

	UpdateSkipButtonState();
}

void UWildHUDWidget::OnLogButtonClicked()
{
	ToggleCombatLog();
}

void UWildHUDWidget::ToggleCombatLog()
{
	if (BP_CombatLogWidget)
	{
		BP_CombatLogWidget->ToggleVisibility();
	}
}

FReply UWildHUDWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 카드 선택 모드 실행 중 확인 버튼 외 다른 화면 클릭 시 기능 취소
		if (bIsCardSelectionActive)
		{
			if (ActiveSelectionContext.bAllowCancel)
			{
				HandleHandCardSelectionCancelled();
			}
			return FReply::Handled();
		}
		// 카드 외 화면(HUD 영역)을 클릭했을 때 손패가 올라와 있다면 안쪽으로 넣음
		if (HandWidget && HandWidget->IsHandExpanded())
		{
			HandWidget->SetHandExpanded(false);
		}

		// 액션 메뉴(SelectionWidget 또는 PlayerSelectionWidget)가 열려 있는 상태에서 위젯 바깥 화면 클릭 시 메뉴 닫기
		auto IsClickedInside = [&](UWidget* Widget) -> bool
		{
			return Widget && Widget->IsVisible() && Widget->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
		};

		bool bAnyMenuVisible = (BP_SelectActionWidget && BP_SelectActionWidget->IsVisible()) ||
		                       (BP_PlayerSelectActionWidget && BP_PlayerSelectActionWidget->IsVisible());

		if (bAnyMenuVisible && !IsClickedInside(BP_SelectActionWidget) && !IsClickedInside(BP_PlayerSelectActionWidget))
		{
			if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
			{
				PC->CancelTileAction();
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWildHUDWidget::InitializeHUD(UWildHUDViewModel* InViewModel)
{
	if (!InViewModel) 
	{
		return;
	}

	HUDViewModel = InViewModel;

	// MVVM View 익스텐션에 뷰모델 등록 (UMG 디자이너의 수동(Manual) 생성 모드 바인딩 활성화)
	if (UMVVMView* View = GetExtension<UMVVMView>())
	{
		View->SetViewModel(FName("WildHUDViewModel"), InViewModel);
	}

	// 하위 위젯인 손패 UI에 뷰모델을 전달하여 스스로 초기화하도록 명령합니다.
	if (HandWidget)
	{
		HandWidget->InitializeHandWidget(HUDViewModel);
		HandWidget->OnCardSelectionCountChanged.RemoveAll(this);
		HandWidget->OnCardSelectionCountChanged.AddUObject(this, &UWildHUDWidget::HandleHandCardSelectionCountChanged);
		HandWidget->OnCardSelectionCancelledDelegate.Unbind();
		HandWidget->OnCardSelectionCancelledDelegate.BindUObject(this, &UWildHUDWidget::HandleHandCardSelectionCancelled);
		HandWidget->OnSearchLibraryCardSelected.Unbind();
		HandWidget->OnSearchLibraryCardSelected.BindUObject(this, &UWildHUDWidget::HandleHandCardSelectedForSearchLibrary);
		HandWidget->OnSearchLibraryCancelled.Unbind();
		HandWidget->OnSearchLibraryCancelled.BindUObject(this, &UWildHUDWidget::HandleHandCancelledForSearchLibrary);
	}

	if (EnemyHandWidget)
	{
		EnemyHandWidget->InitializeEnemyHandWidget(HUDViewModel);
	}

	if (BP_CombatLogWidget)
	{
		BP_CombatLogWidget->InitializeCombatLogWidget(HUDViewModel);
	}

	if (UWorld* World = GetWorld())
	{
		if (AInGameState* GS = World->GetGameState<AInGameState>())
		{
			GS->OnVisualCueReceivedDelegate.RemoveDynamic(this, &UWildHUDWidget::HandleVisualCue);
			GS->OnVisualCueReceivedDelegate.AddDynamic(this, &UWildHUDWidget::HandleVisualCue);
		}
	}

	UpdateSkipButtonState();
}

void UWildHUDWidget::UpdateMana(int32 InMana)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetMyMana(InMana);
	}

	if (Text_Mana)
	{
		Text_Mana->SetText(FText::Format(FText::FromString(TEXT("MANA: {0}")), FText::AsNumber(InMana)));
	}
}

void UWildHUDWidget::UpdateTimer(int32 RemainTime)
{
	if (HUDViewModel)
	{
		HUDViewModel->SetRemainTime(RemainTime);
	}

	if (Timer)
	{
		Timer->SetText(FText::AsNumber(RemainTime));
	}
}

void UWildHUDWidget::ShowTargetingFocus(const FText& PromptMessage)
{
	// 3D 월드 타겟팅(이동, 전투 등) 시 전체 2D 화면을 어둡게(Dim) 하면
	// 선택 가능한 3D 타일까지 함께 어두워지는 시각적 문제가 발생하므로
	// 뒷배경 딤(Border_SearchLibraryDim)은 활성화하지 않고 상단 안내 문구만 표시합니다.
	if (Border_SearchLibraryDim)
	{
		Border_SearchLibraryDim->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_SearchLibraryPrompt)
	{
		Text_SearchLibraryPrompt->SetText(PromptMessage);
		Text_SearchLibraryPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWildHUDWidget::HideTargetingFocus()
{
	if (Border_SearchLibraryDim)
	{
		Border_SearchLibraryDim->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_SearchLibraryPrompt)
	{
		Text_SearchLibraryPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWildHUDWidget::RequestCardSelection(const FWildCardSelectionContext& Context)
{
	ActiveSelectionContext = Context;
	bIsCardSelectionActive = true;

	if (Border_SearchLibraryDim)
	{
		Border_SearchLibraryDim->SetVisibility(ESlateVisibility::Visible);
	}

	if (Text_SearchLibraryPrompt)
	{
		Text_SearchLibraryPrompt->SetText(Context.PromptText);
		Text_SearchLibraryPrompt->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Text_SearchLibraryConfirm)
	{
		Text_SearchLibraryConfirm->SetText(Context.ConfirmButtonText);
	}

	if (HandWidget)
	{
		if (UCanvasPanelSlot* HandSlot = Cast<UCanvasPanelSlot>(HandWidget->Slot))
		{
			HandSlot->SetZOrder(100);
		}
		HandWidget->StartCardSelectionMode(Context.MinSelectionCount, Context.MaxSelectionCount);
	}
}

void UWildHUDWidget::CloseCardSelection()
{
	bIsCardSelectionActive = false;

	if (Border_SearchLibraryDim)
	{
		Border_SearchLibraryDim->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_SearchLibraryPrompt)
	{
		Text_SearchLibraryPrompt->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (HandWidget)
	{
		HandWidget->EndCardSelectionMode();
	}
}

void UWildHUDWidget::HandleHandCardSelectionCountChanged(int32 CurrentSelectedCount)
{
	if (!Button_SearchLibraryConfirm) return;

	bool bCanConfirm = (CurrentSelectedCount >= ActiveSelectionContext.MinSelectionCount &&
	                    CurrentSelectedCount <= ActiveSelectionContext.MaxSelectionCount);

	Button_SearchLibraryConfirm->SetVisibility(bCanConfirm ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Button_SearchLibraryConfirm->SetIsEnabled(bCanConfirm);
}

void UWildHUDWidget::HandleHandCardSelectionCancelled()
{
	FOnCardSelectionCancelled CancelCallback = ActiveSelectionContext.OnCancelledDelegate;
	CloseCardSelection();
	CancelCallback.ExecuteIfBound();
}

void UWildHUDWidget::OnCardSelectionConfirmClicked()
{
	if (!bIsCardSelectionActive || !HandWidget) return;

	TArray<int32> SelectedIDs = HandWidget->GetSelectedCardInstanceIDs();
	if (SelectedIDs.Num() < ActiveSelectionContext.MinSelectionCount ||
	    SelectedIDs.Num() > ActiveSelectionContext.MaxSelectionCount)
	{
		return;
	}

	for (int32 CardID : SelectedIDs)
	{
		HandWidget->SetCardInstanceDisabled(CardID, true);
	}

	FOnCardSelectionConfirmed ConfirmCallback = ActiveSelectionContext.OnConfirmedDelegate;
	CloseCardSelection();
	ConfirmCallback.ExecuteIfBound(SelectedIDs);
}

void UWildHUDWidget::ShowTacticalSkillCardSelection(UTacticalSkillBase* Skill)
{
	FWildCardSelectionContext Context;
	Context.PromptText = FText::FromString(TEXT("패에서 카드 한장을 선택하세요"));
	Context.ConfirmButtonText = FText::FromString(TEXT("확인"));
	Context.MinSelectionCount = 1;
	Context.MaxSelectionCount = 1;

	AInGameController* PC = Cast<AInGameController>(GetOwningPlayer());
	Context.OnConfirmedDelegate.BindLambda([this, PC](const TArray<int32>& SelectedIDs)
	{
		if (PC && SelectedIDs.Num() > 0)
		{
			PC->ConfirmTacticalSkillCardSelection(SelectedIDs[0]);
		}
	});

	Context.OnCancelledDelegate.BindLambda([PC]()
	{
		if (PC)
		{
			PC->CancelTacticalSkillCardSelection();
		}
	});

	RequestCardSelection(Context);
}

void UWildHUDWidget::HideTacticalSkillCardSelection()
{
	CloseCardSelection();
}

void UWildHUDWidget::HandleHandCardSelectedForSearchLibrary(UWildCardWidget* Card)
{
	if (Button_SearchLibraryConfirm)
	{
		Button_SearchLibraryConfirm->SetVisibility(ESlateVisibility::Visible);
	}
}

void UWildHUDWidget::OnSkipButtonClicked()
{
	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		PC->CancelTileAction();
		PC->SkipCurrentAction();
	}
}

void UWildHUDWidget::UpdateSkipButtonState()
{
	AInGameController* PC = Cast<AInGameController>(GetOwningPlayer());
	if (!PC) return;

	AInGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
	if (!GameState || !PS) return;

	EWildMatchState MatchState = GameState->GetCurrentMatchState();
	EWildCardPhase Phase = GameState->GetCurrentPhase();
	int32 MyPlayerID = PS->GetPlayerId();

	bool bVisible = false;
	bool bEnabled = false;
	FText ButtonText = FText::FromString(TEXT("스킵 / 패스"));

	if (MatchState == EWildMatchState::MainGame)
	{
		if (Phase == EWildCardPhase::Action_Private)
		{
			bVisible = true;
			if (PS->bIsActionReady)
			{
				bEnabled = false;
				ButtonText = FText::FromString(TEXT("상대 대기 중..."));
			}
			else
			{
				bEnabled = true;
				ButtonText = FText::FromString(TEXT("행동 스킵"));
			}
		}
		else if (Phase == EWildCardPhase::Action_Public)
		{
			bVisible = true;
			int32 PromptID = GameState->GetCurrentChainPromptID();
			if (PromptID == MyPlayerID)
			{
				bEnabled = true;
				ButtonText = FText::FromString(TEXT("체인 패스"));
			}
			else
			{
				bEnabled = false;
				ButtonText = FText::FromString(TEXT("체인 대기 중..."));
			}
		}
		else
		{
			bVisible = false;
			bEnabled = false;
		}
	}
	else
	{
		bVisible = false;
		bEnabled = false;
	}

	if (Button_SkipAction)
	{
		Button_SkipAction->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_SkipAction->SetIsEnabled(bEnabled);
	}

	if (Text_SkipAction)
	{
		Text_SkipAction->SetText(ButtonText);
	}

	if (HUDViewModel)
	{
		HUDViewModel->SetSkipButtonVisible(bVisible);
		HUDViewModel->SetSkipButtonEnabled(bEnabled);
		HUDViewModel->SetSkipButtonText(ButtonText);
	}
}

void UWildHUDWidget::HandleSelectMenu()
{
	if (BP_SelectActionWidget)
	{
		if (BP_SelectActionWidget->GetVisibility() == ESlateVisibility::Visible)
		{
			BP_SelectActionWidget->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogTemp, Log, TEXT("Select Action Widget Closed"));
		}
		else
		{
			BP_SelectActionWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogTemp, Log, TEXT("Select Action Widget Opened"));
		}
	}
	if (BP_PlayerSelectActionWidget)
	{
		BP_PlayerSelectActionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UUserWidget* UWildHUDWidget::ShowTimingNotice(EWildTimingNoticeType NoticeType, const FText& CustomMessage, float InDuration)
{
	if (NoticeType == EWildTimingNoticeType::None)
	{
		return nullptr;
	}

	// 이미 화면에 알림이 떠 있다면 큐에 적재하여 순차 재생
	if (CurrentNoticeWidget != nullptr)
	{
		FWildNoticeQueueItem QueueItem;
		QueueItem.NoticeType = NoticeType;
		QueueItem.CustomMessage = CustomMessage;
		QueueItem.InDuration = InDuration;
		if (PendingNoticeQueue.Num() < 4)
		{
			PendingNoticeQueue.Add(QueueItem);
		}
		return nullptr;
	}

	float FinalDuration = (InDuration > 0.0f) ? InDuration : GetDurationForTiming(NoticeType);
	FText FinalMessage = !CustomMessage.IsEmpty() ? CustomMessage : GetDefaultMessageForTiming(NoticeType);
	TSubclassOf<UUserWidget> TargetClass = GetWidgetClassForTiming(NoticeType);

	if (!TargetClass)
	{
		TargetClass = UWildNoticeWidget::StaticClass();
	}

	APlayerController* OwningPC = GetOwningPlayer();
	if (!OwningPC)
	{
		OwningPC = Cast<APlayerController>(GetOuter());
	}
	UUserWidget* NewWidget = OwningPC ? CreateWidget<UUserWidget>(OwningPC, TargetClass) : CreateWidget<UUserWidget>(GetWorld(), TargetClass);
	if (!NewWidget)
	{
		return nullptr;
	}

	// 컨테이너가 바인딩되어 있다면 컨테이너 내부에 자식으로 추가, 없다면 뷰포트 최상단에 추가
	// 컨테이너가 바인딩되어 있고 HUD가 뷰포트에 추가되어 있다면 컨테이너 내부에 자식으로 추가, 아니라면 뷰포트 최상단에 직접 추가
	if (NoticeContainer && IsInViewport())
	{
		NoticeContainer->AddChild(NewWidget);
	}
	else
	{
		NewWidget->AddToViewport(100);
	}

	// UWildNoticeWidget 계열인 경우 ShowNotice 호출 및 닫힘 이벤트 연결
	if (UWildNoticeWidget* WildNotice = Cast<UWildNoticeWidget>(NewWidget))
	{
		WildNotice->OnNoticeDismissed.AddDynamic(this, &UWildHUDWidget::HideCurrentNotice);
		WildNotice->ShowNotice(NoticeType, FinalMessage, FinalDuration);
	}
	else
	{
		// 디자이너가 만든 일반 UUserWidget인 경우 지정 시간 후 자동 제거 타이머 등록
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				NoticeDismissTimerHandle,
				this,
				&UWildHUDWidget::HideCurrentNotice,
				FinalDuration,
				false
			);
		}
	}

	CurrentNoticeWidget = NewWidget;

	BP_OnTimingNoticeTriggered(NoticeType, FinalMessage);

	return NewWidget;
}

void UWildHUDWidget::HideCurrentNotice()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoticeDismissTimerHandle);
	}

	if (CurrentNoticeWidget)
	{
		if (UWildNoticeWidget* WildNotice = Cast<UWildNoticeWidget>(CurrentNoticeWidget))
		{
			WildNotice->OnNoticeDismissed.RemoveDynamic(this, &UWildHUDWidget::HideCurrentNotice);
		}
		CurrentNoticeWidget->RemoveFromParent();
		CurrentNoticeWidget = nullptr;
	}

	// 대기열에 다음 알림이 있다면 즉시 재생
	if (PendingNoticeQueue.Num() > 0)
	{
		FWildNoticeQueueItem NextItem = PendingNoticeQueue[0];
		PendingNoticeQueue.RemoveAt(0);
		ShowTimingNotice(NextItem.NoticeType, NextItem.CustomMessage, NextItem.InDuration);
	}
}

TSubclassOf<UUserWidget> UWildHUDWidget::GetWidgetClassForTiming(EWildTimingNoticeType NoticeType) const
{
	// 1. 개별 명시적 프로퍼티 우선 확인
	switch (NoticeType)
	{
	case EWildTimingNoticeType::MainActionReveal:
		if (MainActionRevealWidgetClass) return MainActionRevealWidgetClass;
		break;
	case EWildTimingNoticeType::MainActionResolution:
		if (MainActionResolutionWidgetClass) return MainActionResolutionWidgetClass;
		break;
	case EWildTimingNoticeType::TurnStart:
		if (TurnStartWidgetClass) return TurnStartWidgetClass;
		break;
	case EWildTimingNoticeType::ActionPhase:
		if (ActionPhaseWidgetClass) return ActionPhaseWidgetClass;
		break;
	case EWildTimingNoticeType::ChainStart:
		if (ChainStartWidgetClass) return ChainStartWidgetClass;
		break;
	case EWildTimingNoticeType::ChainResolution:
		if (ChainResolutionWidgetClass) return ChainResolutionWidgetClass;
		break;
	case EWildTimingNoticeType::TurnEnd:
		if (TurnEndWidgetClass) return TurnEndWidgetClass;
		break;
	case EWildTimingNoticeType::Victory:
		if (VictoryWidgetClass) return VictoryWidgetClass;
		break;
	case EWildTimingNoticeType::Defeat:
		if (DefeatWidgetClass) return DefeatWidgetClass;
		break;
	default:
		break;
	}

	// 2. 맵에 설정된 클래스 확인
	if (const TSubclassOf<UUserWidget>* FoundClass = TimingWidgetMap.Find(NoticeType))
	{
		if (*FoundClass) return *FoundClass;
	}

	// 3. 공통 기본 위젯 클래스 확인
	if (DefaultNoticeWidgetClass)
	{
		return DefaultNoticeWidgetClass;
	}

	return UWildNoticeWidget::StaticClass();
}

float UWildHUDWidget::GetDurationForTiming(EWildTimingNoticeType NoticeType) const
{
	if (const float* FoundDuration = TimingNoticeDurations.Find(NoticeType))
	{
		return *FoundDuration;
	}
	return DefaultNoticeDuration;
}

FText UWildHUDWidget::GetDefaultMessageForTiming(EWildTimingNoticeType NoticeType) const
{
	switch (NoticeType)
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

TSubclassOf<UWildVisualEffectWidget> UWildHUDWidget::GetWidgetClassForCueTag(const FGameplayTag& CueTag) const
{
	if (!CueTag.IsValid())
	{
		return DefaultVisualEffectWidgetClass;
	}

	// 1. 정확한 태그 매칭 시도
	if (const TSubclassOf<UWildVisualEffectWidget>* FoundClass = VisualEffectWidgetMap.Find(CueTag))
	{
		if (*FoundClass)
		{
			return *FoundClass;
		}
	}

	// 2. 계층형 태그 폴백 검색 (예: Visual.Effect.Damage.Fire -> Visual.Effect.Damage -> Visual.Effect)
	FGameplayTag ParentTag = CueTag.RequestDirectParent();
	while (ParentTag.IsValid())
	{
		if (const TSubclassOf<UWildVisualEffectWidget>* FoundClass = VisualEffectWidgetMap.Find(ParentTag))
		{
			if (*FoundClass)
			{
				return *FoundClass;
			}
		}
		ParentTag = ParentTag.RequestDirectParent();
	}

	return DefaultVisualEffectWidgetClass;
}

void UWildHUDWidget::HandleVisualCue(const FWildVisualCueData& CueData)
{
	SpawnVisualEffect(CueData);
}

UWildVisualEffectWidget* UWildHUDWidget::SpawnVisualEffect(const FWildVisualCueData& CueData)
{
	TSubclassOf<UWildVisualEffectWidget> WidgetClassToSpawn = GetWidgetClassForCueTag(CueData.CueTag);

	UWildVisualEffectWidget* SpawnedWidget = nullptr;
	if (WidgetClassToSpawn)
	{
		SpawnedWidget = CreateWidget<UWildVisualEffectWidget>(this, WidgetClassToSpawn);
		if (SpawnedWidget)
		{
			if (VisualEffectContainer)
			{
				VisualEffectContainer->AddChild(SpawnedWidget);
			}
			else
			{
				SpawnedWidget->AddToViewport(40);
			}

			SpawnedWidget->PlayVisualEffect(CueData);
		}
	}

	BP_OnVisualCueTriggered(CueData, SpawnedWidget);
	return SpawnedWidget;
}
