#include "InGame/UI/Widget/SelectActionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InGameController.h"
#include "InGamePlayerState.h"
#include "InGameState.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Battle/PlayerStatComponent.h"
#include "InGame/Data/BattleData.h"
#include "InGame/Effect/TileSelectComponent.h"

void USelectActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// UMG 블루프린트 에셋에서 위젯 변수명 뒤에 공백이 포함된 경우("Button_Move ", "Button_TacticalSkill ") 대비 바인딩 보정
	if (!Button_Move)
	{
		Button_Move = Cast<UButton>(GetWidgetFromName(TEXT("Button_Move ")));
	}
	if (!Button_TacticalSkill)
	{
		Button_TacticalSkill = Cast<UButton>(GetWidgetFromName(TEXT("Button_TacticalSkill ")));
	}
	if (!Button_Combat)
	{
		Button_Combat = Cast<UButton>(GetWidgetFromName(TEXT("Button_Combat ")));
	}
	if (!Button_Effect)
	{
		Button_Effect = Cast<UButton>(GetWidgetFromName(TEXT("Button_Effect ")));
	}

	if (!Text_Move)
	{
		Text_Move = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_Move")));
	}
	if (!Text_TacticalSkill)
	{
		Text_TacticalSkill = Cast<UTextBlock>(GetWidgetFromName(TEXT("Text_TacticalSkill")));
	}

	// UMG 디자이너에서 버튼이 아직 배치되지 않은 경우를 대비한 동적 위젯 트리 구성 (모든 버튼이 미배치된 경우에만 실행)
	if (!Button_Move && !Button_TacticalSkill && !Button_Combat && !Button_Effect)
	{
		UCanvasPanel* RootCanvas = nullptr;
		if (WidgetTree)
		{
			RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
			if (!RootCanvas)
			{
				RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DynamicRootCanvas"));
				WidgetTree->RootWidget = RootCanvas;
			}
		}

		if (RootCanvas && WidgetTree)
		{
			UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DynamicMenuBox"));
			if (MenuBox)
			{
				if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MenuBox))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
					CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					CanvasSlot->SetAutoSize(true);
				}

				// 1. Move 버튼 생성
				if (!Button_Move)
				{
					Button_Move = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Move_Dynamic"));
					Text_Move = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Move_Dynamic"));
					if (Button_Move && Text_Move)
					{
						Text_Move->SetText(FText::FromString(TEXT("이동 (Move)")));
						Text_Move->SetJustification(ETextJustify::Center);
						FSlateFontInfo FontInfo = Text_Move->GetFont();
						FontInfo.Size = 14;
						Text_Move->SetFont(FontInfo);

						Button_Move->AddChild(Text_Move);
						if (UVerticalBoxSlot* VSlot = MenuBox->AddChildToVerticalBox(Button_Move))
						{
							VSlot->SetPadding(FMargin(4.0f, 4.0f));
							VSlot->SetHorizontalAlignment(HAlign_Fill);
						}
					}
				}

				// 2. TacticalSkill 버튼 생성
				if (!Button_TacticalSkill)
				{
					Button_TacticalSkill = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_TacticalSkill_Dynamic"));
					Text_TacticalSkill = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_TacticalSkill_Dynamic"));
					if (Button_TacticalSkill && Text_TacticalSkill)
					{
						Text_TacticalSkill->SetText(FText::FromString(TEXT("전술 스킬")));
						Text_TacticalSkill->SetJustification(ETextJustify::Center);
						FSlateFontInfo FontInfo = Text_TacticalSkill->GetFont();
						FontInfo.Size = 14;
						Text_TacticalSkill->SetFont(FontInfo);

						Button_TacticalSkill->AddChild(Text_TacticalSkill);
						if (UVerticalBoxSlot* VSlot = MenuBox->AddChildToVerticalBox(Button_TacticalSkill))
						{
							VSlot->SetPadding(FMargin(4.0f, 4.0f));
							VSlot->SetHorizontalAlignment(HAlign_Fill);
						}
					}
				}
			}
		}
	}

	if (Button_Move)
	{
		Button_Move->OnClicked.RemoveDynamic(this, &USelectActionWidget::OnMoveButtonClicked);
		Button_Move->OnClicked.AddDynamic(this, &USelectActionWidget::OnMoveButtonClicked);
	}
	if (Button_Combat)
	{
		Button_Combat->OnClicked.RemoveDynamic(this, &USelectActionWidget::OnCombatButtonClicked);
		Button_Combat->OnClicked.AddDynamic(this, &USelectActionWidget::OnCombatButtonClicked);
	}
	if (Button_Effect)
	{
		Button_Effect->OnClicked.RemoveDynamic(this, &USelectActionWidget::OnEffectButtonClicked);
		Button_Effect->OnClicked.AddDynamic(this, &USelectActionWidget::OnEffectButtonClicked);
	}
	if (Button_TacticalSkill)
	{
		Button_TacticalSkill->OnClicked.RemoveDynamic(this, &USelectActionWidget::OnTacticalSkillButtonClicked);
		Button_TacticalSkill->OnClicked.AddDynamic(this, &USelectActionWidget::OnTacticalSkillButtonClicked);
	}
}

void USelectActionWidget::OnMoveButtonClicked()
{
	OnMoveButtonPressed.Broadcast();
	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
		{
			UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnMoveButtonClicked - PlayerID: %d"), PS->GetPlayerId());
		}
	}
}

void USelectActionWidget::OnCombatButtonClicked()
{
	OnCombatButtonPressed.Broadcast();
	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
		{
			UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnCombatButtonClicked - PlayerID: %d"), PS->GetPlayerId());
		}
	}
}

void USelectActionWidget::OnEffectButtonClicked()
{
	AInGameController* PC = Cast<AInGameController>(GetOwningPlayer());

	// 현재 선택된 유닛이 PlayerUnit이고 별도의 TacticalSkill 버튼이 없는 경우에만 전술 스킬 버튼으로 폴백 작동
	if (PC && PC->GetTileSelectComponent())
	{
		if (APlayerUnit* PlayerUnit = Cast<APlayerUnit>(PC->GetTileSelectComponent()->GetSelectedUnit()))
		{
			if (!Button_TacticalSkill)
			{
				OnTacticalSkillButtonClicked();
				return;
			}
		}
	}

	// [검증] Effect는 서브 행동이므로 비공개 행동 결정 단계(Action_Private)에서는 선택/실행 불가
	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (GS && GS->GetCurrentPhase() == EWildCardPhase::Action_Private)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectActionWidget] Effect는 서브 행동이므로 Action_Private 페이즈에서 선택할 수 없습니다."));
		return;
	}

	OnEffectButtonPressed.Broadcast();
	if (PC)
	{
		if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
		{
			UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnEffectButtonClicked - PlayerID: %d"), PS->GetPlayerId());
		}
	}
}

void USelectActionWidget::OnTacticalSkillButtonClicked()
{
	OnTacticalSkillButtonPressed.Broadcast();
	if (AInGameController* PC = Cast<AInGameController>(GetOwningPlayer()))
	{
		if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
		{
			UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnTacticalSkillButtonClicked - PlayerID: %d"), PS->GetPlayerId());
		}
	}
}

void USelectActionWidget::SetupForPlayerUnit(APlayerUnit* InPlayerUnit, EWildCardPhase CurrentPhase, bool bHasChainPriority)
{
	// 1. 공격(Combat) 버튼은 PlayerUnit 액션 메뉴에서는 숨김
	if (Button_Combat)
	{
		Button_Combat->SetVisibility(ESlateVisibility::Collapsed);
	}

	AInGameController* PC = Cast<AInGameController>(GetOwningPlayer());
	AInGamePlayerState* PS = PC ? PC->GetPlayerState<AInGamePlayerState>() : nullptr;

	// 2. 이동(Move) 버튼 설정: 메인 액션(Action_Private) 페이즈이고 미레디 상태일 때만 활성화
	if (Button_Move)
	{
		Button_Move->SetVisibility(ESlateVisibility::Visible);
		bool bCanMove = (CurrentPhase == EWildCardPhase::Action_Private) && (PS && !PS->bIsActionReady);
		Button_Move->SetIsEnabled(bCanMove);
	}

	// 3. 전술 스킬 버튼이 존재할 경우 일반 Effect 버튼은 반드시 숨김
	if (Button_TacticalSkill && Button_Effect)
	{
		Button_Effect->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 4. 전술 스킬(Tactical Skill) 버튼 설정: Button_TacticalSkill 또는 Button_Effect 폴백 사용
	UButton* ActiveSkillBtn = Button_TacticalSkill ? Button_TacticalSkill.Get() : Button_Effect.Get();
	if (ActiveSkillBtn)
	{
		ActiveSkillBtn->SetVisibility(ESlateVisibility::Visible);

		UTacticalSkillBase* Skill = InPlayerUnit ? InPlayerUnit->GetTacticalSkill() : nullptr;
		int32 CurrentMana = (PS && PS->GetStatComponent()) ? PS->GetStatComponent()->Mana : 0;
		bool bHasMana = Skill ? (CurrentMana >= Skill->ManaCost) : false;
		bool bNotUsed = InPlayerUnit ? !InPlayerUnit->IsTacticalSkillUsedThisTurn() : false;

		bool bCanInitiate = Skill ? Skill->CanInitiate(InPlayerUnit) : false;

		// 전술 스킬은 Chain 전용 SubAction: Action_Public 페이즈 & 본인 체인 우선권 보유 시에만 활성화
		bool bCanUseSkill = (CurrentPhase == EWildCardPhase::Action_Public) && bHasChainPriority && bHasMana && bNotUsed && bCanInitiate;
		ActiveSkillBtn->SetIsEnabled(bCanUseSkill);

		if (Text_TacticalSkill && Skill)
		{
			Text_TacticalSkill->SetText(FText::Format(FText::FromString(TEXT("{0} ({1})")), Skill->SkillName, FText::AsNumber(Skill->ManaCost)));
		}
	}
}

void USelectActionWidget::SetupForGeneralUnit(AUnitBase* InUnit, EWildCardPhase CurrentPhase, bool bHasChainPriority)
{
	AInGameController* PC = Cast<AInGameController>(GetOwningPlayer());
	AInGamePlayerState* PS = PC ? PC->GetPlayerState<AInGamePlayerState>() : nullptr;

	if (CurrentPhase == EWildCardPhase::None)
	{
		if (AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr)
		{
			CurrentPhase = GS->GetCurrentPhase();
			if (PS)
			{
				bHasChainPriority = (GS->GetCurrentChainPromptID() == PS->GetPlayerId());
			}
		}
	}

	// 1. 이동(Move) 및 공격(Combat) 버튼 설정: 메인 액션(Action_Private) 페이즈이고 미레디 상태일 때만 활성화
	const bool bCanMainAction = (CurrentPhase == EWildCardPhase::Action_Private) && (PS && !PS->bIsActionReady);

	if (Button_Move)
	{
		Button_Move->SetVisibility(ESlateVisibility::Visible);
		Button_Move->SetIsEnabled(bCanMainAction);
	}
	if (Button_Combat)
	{
		Button_Combat->SetVisibility(ESlateVisibility::Visible);
		Button_Combat->SetIsEnabled(bCanMainAction);
	}

	if (Button_TacticalSkill)
	{
		Button_TacticalSkill->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 2. 효과(Effect) 버튼 설정: Effect는 서브 행동이므로 비공개 행동 결정 단계(Action_Private)에서는 선택 불가!
	//    공개 체인 단계(Action_Public)에서 체인 우선권을 보유한 경우에만 활성화
	if (Button_Effect)
	{
		Button_Effect->SetVisibility(ESlateVisibility::Visible);
		const bool bCanUseEffect = (CurrentPhase == EWildCardPhase::Action_Public) && bHasChainPriority;
		Button_Effect->SetIsEnabled(bCanUseEffect);
	}
}

void USelectActionWidget::SetPositionFromWorldLocation(FVector WorldLocation)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FVector2D ScreenPos;
	bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, WorldLocation, ScreenPos, false);
	if (bProjected)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
		{
			CanvasSlot->SetPosition(ScreenPos);
			CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		}
		SetRenderOpacity(1.0f);
	}
	else
	{
		SetRenderOpacity(0.0f);
	}
}
