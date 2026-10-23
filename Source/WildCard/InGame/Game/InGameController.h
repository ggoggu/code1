// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattleData.h"
#include "InGame/UI/Widget/WildCardSelectionTypes.h"
#include "InGameController.generated.h"

class UWildCardViewModel;
class ATile;
class USelectActionWidget;

DECLARE_DELEGATE_OneParam(FOnTileTargetSelected, class ATile* /*SelectedTile*/);
DECLARE_DELEGATE(FOnTileTargetCancelled);

/**
 * 
 */

UCLASS()
class WILDCARD_API AInGameController : public APlayerController
{
	GENERATED_BODY()

public:
	AInGameController();

	UFUNCTION(BlueprintCallable)
	void CompleteMulligan();

	UFUNCTION(BlueprintCallable)
	void PlayChainCard(const FWildActionData& ActionData);

	UFUNCTION(BlueprintCallable)
	void PassChain();

	UFUNCTION(BlueprintCallable)
	void CompleteGameset();

	UFUNCTION(BlueprintCallable)
	void CompletePrivateAction(const FWildActionData& ActionData);

	UFUNCTION(BlueprintCallable, Category = "Action")
	void SkipCurrentAction();

	void RefreshMulliganUI(const TArray<FWildCardDisplayData>& NewCards);

protected:

	FTimerHandle BindTimer;
	
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

	// 다양한 케이스 테스트용으로 불피요시 지울 수 ㅣㅇㅆ음
	void TryBindGameState();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitAction(FWildActionData ActionData);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteMulligan();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteGameset();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DevSetMana(int32 NewMana);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitSelectedDeck(const FWildDeckData& InDeckData);
	
	UFUNCTION()
	void OnMatchStateChanged(EWildMatchState NewState);
	
	UFUNCTION()
	void OnGamePhaseChanged(EWildCardPhase NewPhase);

	UFUNCTION()
	void OnChainPromptChanged(int32 NewPromptPlayerID);

	UFUNCTION()
	void OnActionReadyChanged(bool bIsReady);
	
	// Debug
public:
	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Ready();
	
	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Pass();

	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Chain();

	// Developer / Cheat
	UFUNCTION(Exec, BlueprintCallable, Category = "WildCard|Developer")
	void Dev_SetMana(int32 NewMana = 10);

	UFUNCTION(Exec, BlueprintCallable, Category = "WildCard|Developer")
	void SetMana10();

	// Widget
public:
	void AttemptPlayCard(UWildCardViewModel* CardViewModel, FVector2D ScreenPosition);

	UPROPERTY(BlueprintReadWrite, Category = "MyCategory")
	FVector2D PointPosition;
	
	UFUNCTION(BlueprintCallable, Category = "Tile|Hover")
	ATile* GetTileUnderScreenPosition(FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Tile|Hover")
	void UpdateTileHover(FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Tile|Hover")
	void UpdateTileHoverUnderCursor();

	UFUNCTION(BlueprintCallable, Category = "Tile|Hover")
	void ClearTileHover();

protected:
	UPROPERTY()
	TWeakObjectPtr<ATile> CurrentHoveredTile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MulliganWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UWildHUDWidget> HUDClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GamesetWidgetClass;

public:
	// --- 타이밍 알림 위젯 클래스 (Controller BP에서 설정 가능) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> MainActionRevealWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> MainActionResolutionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> TurnStartWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> ChainStartWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> ActionPhaseWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> ChainResolutionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> TurnEndWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> VictoryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> DefeatWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TSubclassOf<class UUserWidget> DefaultNoticeWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TMap<EWildTimingNoticeType, TSubclassOf<class UUserWidget>> TimingWidgetMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	float DefaultNoticeDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Notice")
	TMap<EWildTimingNoticeType, float> TimingNoticeDurations;

	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void ShowTimingNotice(EWildTimingNoticeType NoticeType, const FText& CustomMessage = FText::GetEmpty(), float InDuration = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "UI|Notice")
	void HideTimingNotice();

	// HUD 인스턴스 반환 또는 필요 시 지연 생성 (Notice 및 UI 단일 관리 창구)
	UFUNCTION(BlueprintCallable, Category = "UI|HUD")
	class UWildHUDWidget* GetOrCreateHUD();

	UFUNCTION()
	void OnMatchEnded(int32 WinnerPlayerID, int32 DefeatedPlayerID);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UTileSelectComponent* TileSelectComp;

	virtual void OnRep_PlayerState() override;

	FTimerHandle EnemyStateBindTimer;
	void TryBindEnemyState();

	UFUNCTION()
	void OnEnemyHandCountUpdated(int32 NewHandCount);

private:
	UPROPERTY()
	class UUserWidget* MulliganWidgetInstance;
	
	UPROPERTY()
	class UWildHUDWidget* HUDInstance;

	UPROPERTY()
	class UUserWidget* GamesetWidgetInstance;

	UPROPERTY()
	class UWildHUDViewModel* MainHUDViewModel;


	
	void ShowMulliganUI();
	void HideMulliganUI();
	void CreateHUD();
	
	void ShowGamesetUI();
	void HideGamesetUI();

	void ConnectDelegate();

	UFUNCTION()
	void OnTimerUpdated(int32 RemainTime);

	UFUNCTION()
	void OnPlayerManaChanged(int32 NewMana);

	UFUNCTION()
	void UpdateHandFromState(const TArray<FWildCardDisplayData>& HandData);

public:
	// --- Tile Interaction ---
	UFUNCTION()
	void HandleActionMenu();

	UFUNCTION()
	void HandlePlayerUnitActionMenu(class APlayerUnit* InPlayerUnit);

	UFUNCTION()
	void OnTacticalSkillActionSelected();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void CancelTileAction();

	/** 열려 있는 액션 메뉴 위젯의 화면 위치를 유닛 머리 위에 맞춰 실시간 갱신 */
	void UpdateActionMenuPosition();

	/** 테스트/벤치마크용: 유닛 머리 위 WidgetComponent 방식을 사용할지 여부 (false = 2D HUD 투영 방식 [기본], true = 유닛 머리 위 WidgetComponent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|UI|Benchmark")
	bool bTestWidgetComponentMenu = false;

	/** 콘솔 명령어로 실시간 토글: SetActionMenuTestMode <0 또는 1> */
	UFUNCTION(Exec, BlueprintCallable, Category = "WildCard|UI|Benchmark")
	void SetActionMenuTestMode(bool bUseWidgetComp);

	UFUNCTION(BlueprintCallable, Category = "Components")
	class UTileSelectComponent* GetTileSelectComponent() const { return TileSelectComp; }

	void FindCanSpawnTiles();
	void CancledCanSpawnTiles();
	void FindCanMoveTiles(TWeakObjectPtr<ATile> InTile);
	void CancledCanMoveTiles(TWeakObjectPtr<ATile> InTile);
	void FindCanCombatTiles(TWeakObjectPtr<ATile> InTile);
	void CancledCanCombatTiles(TWeakObjectPtr<ATile> InTile);
	void FindCanEffectTiles(TWeakObjectPtr<ATile> InTile);
	void CancledCanEffectTiles(TWeakObjectPtr<ATile> InTile);
	void FindCanTacticalSkillTiles(TWeakObjectPtr<ATile> InTile);
	void CancledCanTacticalSkillTiles(TWeakObjectPtr<ATile> InTile);

	// 범용 3D 타일 타겟팅 시스템 (화면 딤, 타일 발광 하이라이트, 상단 안내 문구, 클릭/취소 콜백)
	void StartTileTargeting(
		const TArray<class ATile*>& InCandidateTiles,
		const FText& InPromptText,
		int32 InTileHighlightState = 1, // 1: Combat(Red), 2: Move(Green), 4: Effect(Purple)
		FOnTileTargetSelected OnSelected = FOnTileTargetSelected(),
		FOnTileTargetCancelled OnCancelled = FOnTileTargetCancelled()
	);

	void CancelTileTargeting();

	UFUNCTION(BlueprintCallable, Category = "Tile Targeting")
	bool IsInTileTargeting() const { return bIsTileTargetingActive; }

	void OnCancelButtonPressed();

	// 범용 카드 선택 요청 (HUD로 중계)
	void RequestCardSelection(const FWildCardSelectionContext& Context);
	void CancelCardSelection();
	bool IsInCardSelection() const;

	// 서고 탐색 등 패 카드 대상 전술 스킬 처리
	void StartTacticalSkillCardSelection(class APlayerUnit* SourceUnit, class UTacticalSkillBase* Skill);
	void CancelTacticalSkillCardSelection();
	void ConfirmTacticalSkillCardSelection(int32 SelectedCardInstanceID);
	bool IsInTacticalSkillCardSelection() const;

	void OnTileSelected();
	void ConnectedInHUD();

	UFUNCTION(Client, Reliable)
	void Client_ChangeMoveableTiles(const TArray<ATile*>& InTiles);

	UFUNCTION(Client, Reliable)
	void Client_ChangeCombatableTiles(const TArray<ATile*>& InTiles);

	UFUNCTION(Client, Reliable)
	void Client_ChangeTacticalSkillTiles(const TArray<ATile*>& InTiles);

	UFUNCTION(Client, Reliable)
	void Client_ChangeEffectableTiles(const TArray<ATile*>& InTiles);

	// 서고 탐색 전술 스킬: 체인 해결 시 카드 선택 프롬프트 요청 (서버 -> 클라이언트)
	UFUNCTION(Client, Reliable)
	void Client_PromptSearchLibrarySelection();

	// 서고 탐색 전술 스킬: 카드 선택 취소/닫기 (서버 -> 클라이언트)
	UFUNCTION(Client, Reliable)
	void Client_CancelCardSelection();

	// 서고 탐색 전술 스킬: 선택된 카드 확인 (클라이언트 -> 서버)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ConfirmSearchLibrarySelection(int32 SelectedCardInstanceID);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CalltoFindMoveableTiles(ATile* InTile, AInGameController* InPlayer);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CalltoFindCombatableTiles(ATile* InTile, AInGameController* InPlayer);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CalltoFindEffectableTiles(ATile* InTile, AInGameController* InPlayer);

protected:
	virtual void SetupInputComponent() override;

private:
	UPROPERTY()
	TObjectPtr<USelectActionWidget> SelectInstance = nullptr;

	UPROPERTY()
	TObjectPtr<USelectActionWidget> PlayerSelectInstance = nullptr;

	UPROPERTY()
	TWeakObjectPtr<class APlayerUnit> PendingTacticalSkillUnit;

	UPROPERTY()
	TWeakObjectPtr<class UTacticalSkillBase> PendingTacticalSkill;

	bool bIsTileTargetingActive = false;
	TArray<TWeakObjectPtr<class ATile>> ActiveTargetingTiles;
	FOnTileTargetSelected ActiveTileSelectedCallback;
	FOnTileTargetCancelled ActiveTileCancelledCallback;

};
