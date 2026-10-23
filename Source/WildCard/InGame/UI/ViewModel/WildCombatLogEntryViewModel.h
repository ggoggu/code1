// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BattleData.h"
#include "WildCombatLogEntryViewModel.generated.h"

/**
 * 개별 전투 로그 항목의 UMG MVVM 뷰모델
 * UListView 또는 커스텀 WBP_CombatLogEntry 위젯과 데이터 바인딩 가능
 */
UCLASS(BlueprintType)
class WILDCARD_API UWildCombatLogEntryViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	int32 LogIndex = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	FText TitleText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	FText FullDetailText;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	EWildCombatLogType Category = EWildCombatLogType::None;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	bool bIsAllyAction = false;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "CombatLog|Entry")
	FLinearColor HeaderColor = FLinearColor(1.0f, 0.88f, 0.45f, 1.0f);

public:
	void InitializeFromEntry(const FWildCombatLogEntry& InEntry, int32 LocalPlayerID, int32 FirstPlayerID);

	void SetTitleText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(TitleText, InText); }
	void SetFullDetailText(const FText& InText) { UE_MVVM_SET_PROPERTY_VALUE(FullDetailText, InText); }
	void SetIsAllyAction(bool bInAlly) { UE_MVVM_SET_PROPERTY_VALUE(bIsAllyAction, bInAlly); }
	void SetHeaderColor(const FLinearColor& InColor) { UE_MVVM_SET_PROPERTY_VALUE(HeaderColor, InColor); }
};
