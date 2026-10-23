// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleData.h"
#include "WildDeckSelectWidget.generated.h"

/**
 * 덱 선택 UMG 위젯(WBP_DeckSelect)의 C++ 베이스 클래스
 */
UCLASS()
class WILDCARD_API UWildDeckSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 특정 덱 선택 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void SelectDeck(const FWildDeckData& InDeck);

	/** 대전 시작 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void RequestStartMatch();

	/** 로그인 화면 복귀 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void RequestReturnToLogin();

	/** 현재 유저의 가용 덱 목록 조회 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	TArray<FWildDeckData> GetAvailableDecks() const;

	/** 현재 선택된 덱 조회 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	FWildDeckData GetCurrentSelectedDeck() const;

	/** 유효한 덱이 선택되었는지 여부 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	bool HasValidSelectedDeck() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 덱 선택이 변경되었을 때 블루프린트에서 UI 애니메이션 및 상세창을 갱신할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Deck")
	void OnDeckSelectionChanged(const FWildDeckData& NewSelectedDeck);

	/** 덱 목록이 갱신되었을 때 블루프린트에서 리스트를 다시 그리는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Deck")
	void OnDeckListUpdated();

private:
	UFUNCTION()
	void HandleDeckSelected(const FWildDeckData& NewSelectedDeck);

	UFUNCTION()
	void HandleDeckListChanged();
};
