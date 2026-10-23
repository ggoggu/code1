// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BattleData.h"
#include "WildHUDViewModel.generated.h"

class UWildCardViewModel;
/**
 * 
 */
UCLASS()
class WILDCARD_API UWildHUDViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- 시스템 및 페이즈 상태 ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|System")
	EWildMatchState CurrentMatchState;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|System")
	EWildCardPhase CurrentTurnPhase;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|System")
	FText TimerText;

	// --- 플레이어 데이터 ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Player")
	int32 MyHealth;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Player")
	int32 MyMana;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Player")
	FText ManaText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Player")
	int32 EnemyHealth;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Enemy")
	int32 EnemyHandCount;

	// --- 손패 리스트 (핵심) ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Hand")
	TArray<UWildCardViewModel*> MyHandCards;

	// --- 스킵 / 패스 버튼 상태 ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Action")
	bool bIsSkipButtonEnabled;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Action")
	bool bIsSkipButtonVisible;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Action")
	FText SkipButtonText;

	// --- 전투 로그 상태 ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|CombatLog")
	bool bIsCombatLogOpen;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|CombatLog")
	int32 CombatLogCount;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|CombatLog")
	FText LatestLogSummary;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|CombatLog")
	TArray<TObjectPtr<class UWildCombatLogEntryViewModel>> CombatLogEntries;

public:
	// 상태 변경 Setter
	void SetMatchState(EWildMatchState InState) { UE_MVVM_SET_PROPERTY_VALUE(CurrentMatchState, InState); }
	void SetTurnPhase(EWildCardPhase InPhase) { UE_MVVM_SET_PROPERTY_VALUE(CurrentTurnPhase, InPhase); }
	void SetTimerText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(TimerText, InText); }
	void SetRemainTime(int32 InRemainTime) { SetTimerText(FText::AsNumber(InRemainTime)); }
	void SetMyHealth(int32 InHealth) { UE_MVVM_SET_PROPERTY_VALUE(MyHealth, InHealth); }
	void SetMyMana(int32 InMana)
	{
		UE_MVVM_SET_PROPERTY_VALUE(MyMana, InMana);
		SetManaText(FText::AsNumber(InMana));
	}
	void SetManaText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(ManaText, InText); }
	void SetEnemyHandCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(EnemyHandCount, InCount); }
	void SetSkipButtonEnabled(bool bInEnabled) { UE_MVVM_SET_PROPERTY_VALUE(bIsSkipButtonEnabled, bInEnabled); }
	void SetSkipButtonVisible(bool bInVisible) { UE_MVVM_SET_PROPERTY_VALUE(bIsSkipButtonVisible, bInVisible); }
	void SetSkipButtonText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(SkipButtonText, InText); }
	
	// 전투 로그 Setter
	void SetCombatLogOpen(bool bInOpen) { UE_MVVM_SET_PROPERTY_VALUE(bIsCombatLogOpen, bInOpen); }
	void ToggleCombatLog() { SetCombatLogOpen(!bIsCombatLogOpen); }
	void SetCombatLogCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(CombatLogCount, InCount); }
	void SetLatestLogSummary(const FText& InSummary) { UE_MVVM_SET_PROPERTY_VALUE(LatestLogSummary, InSummary); }
	void AddCombatLogEntryViewModel(class UWildCombatLogEntryViewModel* InEntryVM);
	void ClearCombatLogEntryViewModels();

	void UpdateMyHandCards(const TArray<UWildCardViewModel*>& InCards);
};
