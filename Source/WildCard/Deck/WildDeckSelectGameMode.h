// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WildDeckSelectGameMode.generated.h"

class AWildDeckSelectPlayerController;

/**
 * 덱 선택 레벨(L_DeckSelect) 전용 게임모드
 */
UCLASS()
class WILDCARD_API AWildDeckSelectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWildDeckSelectGameMode();

	/** 매칭 모달에서 연출 대기가 끝난 플레이어의 매칭 완료 통보 */
	void HandlePlayerMatchReady(APlayerController* PC, const FString& MatchId);

	/** 전용 서버 ServerTravel 실행 */
	void ExecuteMatchServerTravel();

	/** 대전 시작 시 이동할 레벨 경로 (기본값: /Game/Level/start_test) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Deck")
	FName NextLevelPath = FName(TEXT("/Game/Level/start_test"));

private:
	TSet<TWeakObjectPtr<APlayerController>> ReadyPlayers;
	FTimerHandle ForceTravelTimerHandle;
	bool bHasInitiatedTravel = false;
};
