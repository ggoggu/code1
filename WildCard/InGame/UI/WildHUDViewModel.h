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
	int32 EnemyHealth;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Enemy")
	int32 EnemyHandCount;

	// --- 손패 리스트 (핵심) ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "HUD|Hand")
	TArray<UWildCardViewModel*> MyHandCards;

public:
	// 상태 변경 Setter
	void SetMatchState(EWildMatchState InState) { UE_MVVM_SET_PROPERTY_VALUE(CurrentMatchState, InState); }
	void SetTurnPhase(EWildCardPhase InPhase) { UE_MVVM_SET_PROPERTY_VALUE(CurrentTurnPhase, InPhase); }
	void SetMyHealth(int32 InHealth) { UE_MVVM_SET_PROPERTY_VALUE(MyHealth, InHealth); }
	void SetEnemyHandCount(int32 InCount) { UE_MVVM_SET_PROPERTY_VALUE(EnemyHandCount, InCount); }
	
	void UpdateMyHandCards(const TArray<UWildCardViewModel*>& InCards);
	
};
