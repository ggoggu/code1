// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WildMatchmakingTypes.h"
#include "IWildMatchmakingProvider.h"
#include "WildMatchmakingSubsystem.generated.h"

/**
 * 게임인스턴스 전역 매칭 관리 서브시스템
 * 하스스톤 스타일 매칭 대기, 타이머 틱, 매칭 성사 및 인게임 트래블을 총괄합니다.
 */
UCLASS()
class WILDCARD_API UWildMatchmakingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 프로바이더 교체 (추후 AWS / EOS 주입용) */
	void SetProvider(TSharedPtr<IWildMatchmakingProvider> InProvider);

	/** 매칭 시작 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Matchmaking")
	void StartMatchmaking(const FWildDeckData& InDeck, float TimeoutSeconds = 10.0f);

	/** 매칭 취소 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Matchmaking")
	void CancelMatchmaking();

	/** 현재 매칭 상태 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Matchmaking")
	EWildMatchmakingState GetMatchmakingState() const { return CurrentState; }

	/** 현재 매칭 탐색 중인지 여부 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Matchmaking")
	bool IsSearching() const { return CurrentState == EWildMatchmakingState::Searching; }

	/** 마지막 매칭 성사 결과 정보 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Matchmaking")
	const FWildMatchResult& GetLastMatchResult() const { return LastMatchResult; }

	/** 현재 경과 시간(초) 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Matchmaking")
	int32 GetElapsedSeconds() const { return CurrentElapsedSeconds; }

	/** 봇 매칭까지 남은 시간(초) 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Matchmaking")
	int32 GetRemainingSecondsUntilBot() const { return CurrentRemainingSeconds; }

public:
	/** 매칭 상태 변경 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Matchmaking")
	FOnMatchmakingStateChangedSignature OnStateChanged;

	/** 매칭 타이머 틱 (1초 주기, UI 시간 갱신용) */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Matchmaking")
	FOnMatchmakingTimerTickSignature OnTimerTick;

	/** 매칭 성공 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Matchmaking")
	FOnMatchFoundSignature OnMatchFound;

	/** 매칭 에러 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Matchmaking")
	FOnMatchmakingErrorSignature OnMatchmakingError;

private:
	void SetState(EWildMatchmakingState NewState);
	void HandleStatusChanged(EWildMatchmakingState InState);
	void HandleMatchCompleted(bool bSuccess, const FWildMatchResult& Result, const FString& ErrorMessage);
	void HandleMatchCancelled();
	void OnSecondTick();

private:
	TSharedPtr<IWildMatchmakingProvider> ActiveProvider;

	UPROPERTY()
	EWildMatchmakingState CurrentState = EWildMatchmakingState::Idle;

	UPROPERTY()
	FWildMatchResult LastMatchResult;

	UPROPERTY()
	FString ActiveTicketId;

	int32 CurrentElapsedSeconds = 0;
	int32 CurrentRemainingSeconds = 10;
	FTimerHandle SecondTimerHandle;
};
