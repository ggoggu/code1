// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WildMatchmakingTypes.h"
#include "WildMatchmakingWidget.generated.h"

/**
 * 하스스톤 스타일 매칭 모달 UMG 위젯(WBP_MatchmakingDialog)의 C++ 베이스 클래스
 */
UCLASS()
class WILDCARD_API UWildMatchmakingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 매칭 취소 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Matchmaking")
	void RequestCancelMatchmaking();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 매칭 상태 변경 알림 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Matchmaking")
	void OnMatchmakingStateChanged(EWildMatchmakingState NewState);

	/** 매칭 타이머 틱 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Matchmaking")
	void OnMatchmakingTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot);

	/** 매칭 성공 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Matchmaking")
	void OnMatchFound(const FWildMatchResult& MatchResult);

private:
	UFUNCTION()
	void HandleStateChanged(EWildMatchmakingState NewState);

	UFUNCTION()
	void HandleTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot);

	UFUNCTION()
	void HandleMatchFound(const FWildMatchResult& MatchResult);
};
