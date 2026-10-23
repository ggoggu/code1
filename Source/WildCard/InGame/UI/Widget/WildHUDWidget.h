// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleData.h"
#include "InGame/UI/Widget/WildCardSelectionTypes.h"
#include "WildHUDWidget.generated.h"

class UWildHUDViewModel;
class UWildHandWidget;
class USelectActionWidget;
class UButton;
class UTextBlock;
class UBorder;
class UWildVisualEffectWidget;

USTRUCT(BlueprintType)
struct FWildNoticeQueueItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EWildTimingNoticeType NoticeType = EWildTimingNoticeType::None;

	UPROPERTY(BlueprintReadOnly)
	FText CustomMessage;

	UPROPERTY(BlueprintReadOnly)
	float InDuration = -1.0f;
};

UCLASS()
class WILDCARD_API UWildHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// UMG 에디터에서 배치할 하위 손패 위젯 (이름이 반드시 'HandWidget'이어야 바인딩 됨)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWildHandWidget> HandWidget;

	// 이 HUD가 참조하는 최상위 통합 뷰모델
	UPROPERTY(BlueprintReadOnly, Category = "HUD|ViewModel")
	TObjectPtr<UWildHUDViewModel> HUDViewModel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UWildEnemyHandWidget> EnemyHandWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<USelectActionWidget> BP_SelectActionWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<USelectActionWidget> BP_PlayerSelectActionWidget;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Action")
	TSubclassOf<USelectActionWidget> PlayerSelectActionWidgetClass;

	// 행동/체인 스킵(패스) 버튼
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_SkipAction;

	// 행동/체인 스킵(패스) 버튼 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkipAction;

	// 타이머 텍스트 블록 (WBP_InGameHUD 내 위젯 이름이 'Timer'인 경우 자동 바인딩)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Timer;

	// 플레이어 마나 코스트 텍스트 블록 (WBP_InGameHUD 내 위젯 이름이 'Text_Mana'인 경우 자동 바인딩)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Mana;

	// 전투 로그 토글 버튼 (HUD 우측 상단)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CombatLog")
	TObjectPtr<UButton> Button_LogToggle;

	// 전투 로그 토글 버튼 텍스트
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CombatLog")
	TObjectPtr<UTextBlock> Text_LogToggle;

	// 전투 로그 패널 위젯
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CombatLog")
	TObjectPtr<class UWildCombatLogWidget> BP_CombatLogWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|CombatLog")
	TSubclassOf<class UWildCombatLogWidget> CombatLogWidgetClass;

	UFUNCTION()
	void OnSkipButtonClicked();

	UFUNCTION()
	void OnLogButtonClicked();

public:
	// 전투 로그 창 토글
	UFUNCTION(BlueprintCallable, Category = "HUD|CombatLog")
	void ToggleCombatLog();
	// 게임 시작 시 PlayerController에서 호출하여 뷰모델을 주입하는 함수
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeHUD(UWildHUDViewModel* InViewModel);

	void HandleSelectMenu();

	USelectActionWidget* GetSelectActionWidget() const { return BP_SelectActionWidget; }
	USelectActionWidget* GetPlayerSelectActionWidget() const { return BP_PlayerSelectActionWidget; }

	UFUNCTION(BlueprintCallable, Category = "HUD")
	UWildHandWidget* GetHandWidget() const { return HandWidget; }

	// 현재 페이즈 및 턴 권한에 따라 스킵 버튼의 활성화/가시성/텍스트 갱신
	UFUNCTION(BlueprintCallable, Category = "HUD|Action")
	void UpdateSkipButtonState();

	// 마나 UI 갱신 함수
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateMana(int32 InMana);

	// 타이머 UI 갱신 함수
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateTimer(int32 RemainTime);

	// =========================================================================
	// 범용 타겟팅 연출 (화면 딤 & 상단 안내 문구)
	// =========================================================================
public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Targeting")
	void ShowTargetingFocus(const FText& PromptMessage);

	UFUNCTION(BlueprintCallable, Category = "HUD|Targeting")
	void HideTargetingFocus();

	// =========================================================================
	// 범용 카드 선택 UI 모달 시스템
	// =========================================================================
public:
	UFUNCTION(BlueprintCallable, Category = "HUD|CardSelection")
	void RequestCardSelection(const FWildCardSelectionContext& Context);

	UFUNCTION(BlueprintCallable, Category = "HUD|CardSelection")
	void CloseCardSelection();

	UFUNCTION(BlueprintCallable, Category = "HUD|CardSelection")
	bool IsCardSelectionActive() const { return bIsCardSelectionActive; }

	// 하위 호환성 래퍼: 서고 탐색 (전술 스킬) 카드 선택 UI
	UFUNCTION(BlueprintCallable, Category = "HUD|SearchLibrary")
	void ShowTacticalSkillCardSelection(class UTacticalSkillBase* Skill);

	UFUNCTION(BlueprintCallable, Category = "HUD|SearchLibrary")
	void HideTacticalSkillCardSelection();

	UFUNCTION(BlueprintCallable, Category = "HUD|SearchLibrary")
	bool IsTacticalSkillCardSelectionActive() const { return IsCardSelectionActive(); }

protected:
	// 범용 카드 선택 모달 UI (WBP_InGameHUD에서 바인딩 가능)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UBorder> Border_CardSelectionDim;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UTextBlock> Text_CardSelectionPrompt;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UButton> Button_CardSelectionConfirm;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UTextBlock> Text_CardSelectionConfirm;

	// 하위 호환성용 레거시 바인딩 (기존 WBP_InGameHUD 연동 유지)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UBorder> Border_SearchLibraryDim;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UTextBlock> Text_SearchLibraryPrompt;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UButton> Button_SearchLibraryConfirm;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|CardSelection")
	TObjectPtr<UTextBlock> Text_SearchLibraryConfirm;

	FWildCardSelectionContext ActiveSelectionContext;
	bool bIsCardSelectionActive = false;

	UFUNCTION()
	void OnCardSelectionConfirmClicked();

	void HandleHandCardSelectionCountChanged(int32 CurrentSelectedCount);
	void HandleHandCardSelectionCancelled();

	// 하위 호환성 래퍼
	UFUNCTION()
	void OnSearchLibraryConfirmClicked() { OnCardSelectionConfirmClicked(); }
	void HandleHandCardSelectedForSearchLibrary(class UWildCardWidget* Card);
	void HandleHandCancelledForSearchLibrary() { HandleHandCardSelectionCancelled(); }

	// =========================================================================
	// 타이밍 알림 위젯 (턴 시작, 체인 시작 등)
	// =========================================================================
public:
	// 공지 위젯 컨테이너 (WBP_InGameHUD 내 배치 시 여기에 부착, 없을 시 뷰포트에 직접 부착)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|Notice")
	TObjectPtr<class UPanelWidget> NoticeContainer;

	// 타이밍별 커스텀 위젯 클래스 (UMG 에디터 디테일 패널에서 개별 지정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> MainActionRevealWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> MainActionResolutionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> TurnStartWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> ChainStartWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> ActionPhaseWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> ChainResolutionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> TurnEndWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> VictoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> DefeatWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TSubclassOf<class UUserWidget> DefaultNoticeWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Classes")
	TMap<EWildTimingNoticeType, TSubclassOf<class UUserWidget>> TimingWidgetMap;

	// 기본 공지 지속 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Duration")
	float DefaultNoticeDuration = 2.0f;

	// 타이밍별 개별 지속 시간 오버라이드 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|Notice|Duration")
	TMap<EWildTimingNoticeType, float> TimingNoticeDurations;

	// 현재 화면에 표시 중인 공지 위젯 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Notice")
	TObjectPtr<class UUserWidget> CurrentNoticeWidget;

	FTimerHandle NoticeDismissTimerHandle;

	UPROPERTY()
	TArray<FWildNoticeQueueItem> PendingNoticeQueue;

public:
	// 특정 타이밍에 알림 위젯 띄우기 (커스텀 메시지 및 시간 지정 가능)
	UFUNCTION(BlueprintCallable, Category = "HUD|Notice")
	class UUserWidget* ShowTimingNotice(EWildTimingNoticeType NoticeType, const FText& CustomMessage = FText::GetEmpty(), float InDuration = -1.0f);

	// 현재 떠 있는 알림 위젯 닫기
	UFUNCTION(BlueprintCallable, Category = "HUD|Notice")
	void HideCurrentNotice();

	// 타이밍에 맞는 위젯 클래스 반환
	UFUNCTION(BlueprintCallable, Category = "HUD|Notice")
	TSubclassOf<class UUserWidget> GetWidgetClassForTiming(EWildTimingNoticeType NoticeType) const;

	// 타이밍에 맞는 표시 지속 시간 반환
	UFUNCTION(BlueprintCallable, Category = "HUD|Notice")
	float GetDurationForTiming(EWildTimingNoticeType NoticeType) const;

	// 타이밍 기본 메시지 반환
	UFUNCTION(BlueprintCallable, Category = "HUD|Notice")
	FText GetDefaultMessageForTiming(EWildTimingNoticeType NoticeType) const;

	// 블루프린트 연동 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Notice")
	void BP_OnTimingNoticeTriggered(EWildTimingNoticeType NoticeType, const FText& Message);

	// =========================================================================
	// 범용 시각/UI 연출 시스템 (Visual Effect System)
	// =========================================================================
public:
	// 연출 위젯이 부착될 컨테이너 (UMG 에디터에서 BindWidgetOptional, 없을 시 뷰포트에 직접 부착)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "HUD|VisualEffect")
	TObjectPtr<class UPanelWidget> VisualEffectContainer;

	// 태그별 연출 위젯 클래스 매핑 (에디터 디테일 패널에서 설정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|VisualEffect|Classes")
	TMap<FGameplayTag, TSubclassOf<class UWildVisualEffectWidget>> VisualEffectWidgetMap;

	// 기본/폴백 연출 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD|VisualEffect|Classes")
	TSubclassOf<class UWildVisualEffectWidget> DefaultVisualEffectWidgetClass;

	// 연출 브로드캐스트 이벤트 핸들러 (OnVisualCueReceivedDelegate 바인딩용)
	UFUNCTION(BlueprintCallable, Category = "HUD|VisualEffect")
	void HandleVisualCue(const FWildVisualCueData& CueData);

	// 연출 위젯 생성 및 재생 헬퍼
	UFUNCTION(BlueprintCallable, Category = "HUD|VisualEffect")
	class UWildVisualEffectWidget* SpawnVisualEffect(const FWildVisualCueData& CueData);

	// 태그에 매핑된 위젯 클래스 반환 (계층형 태그 폴백 지원)
	UFUNCTION(BlueprintCallable, Category = "HUD|VisualEffect")
	TSubclassOf<class UWildVisualEffectWidget> GetWidgetClassForCueTag(const FGameplayTag& CueTag) const;

	// 블루프린트 연동 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|VisualEffect")
	void BP_OnVisualCueTriggered(const FWildVisualCueData& CueData, class UWildVisualEffectWidget* SpawnedWidget);
};
