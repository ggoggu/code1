// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleData.h"
#include "WildCombatLogWidget.generated.h"

class UScrollBox;
class UButton;
class UTextBlock;
class UVerticalBox;
class UBorder;

UCLASS()
class WILDCARD_API UWildCombatLogWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "CombatLog|UI")
	TObjectPtr<UScrollBox> LogScrollBox;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "CombatLog|UI")
	TObjectPtr<UButton> Button_Close;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "CombatLog|UI")
	TObjectPtr<UTextBlock> Text_Title;

	UFUNCTION()
	void OnCloseButtonClicked();

public:
	// 뷰모델 주입 및 초기화 (MVVM 연동)
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void InitializeCombatLogWidget(class UWildHUDViewModel* InViewModel);

	// 로그 창 보이기/숨기기 토글
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void ToggleVisibility();

	// 전체 로그 갱신
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void RefreshLogs();

	// 단일 로그 항목 수신 처리
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void HandleCombatLogAdded(const FWildCombatLogEntry& NewEntry);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog|ViewModel")
	TObjectPtr<class UWildHUDViewModel> HUDViewModel;

private:
	void AppendLogEntryUI(const FWildCombatLogEntry& Entry);
	void BuildDynamicUIFallback();

	int32 GetLocalPlayerID() const;
	int32 GetFirstPlayerID() const;
};
