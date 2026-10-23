// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IWildMatchmakingProvider.h"
#include "Containers/Ticker.h"

/**
 * 로컬 에디터 2클라이언트 상호 매칭 및 타임아웃 봇 폴백 프로바이더
 * Saved/Matchmaking/LocalMatchQueue.json 파일을 공유 브로커로 사용하여 두 클라이언트를 매칭합니다.
 */
class WILDCARD_API FWildLocalMatchmakingProvider : public IWildMatchmakingProvider
{
public:
	FWildLocalMatchmakingProvider();
	virtual ~FWildLocalMatchmakingProvider() override;

	virtual void Initialize() override;
	virtual void StartMatchmaking(const FWildMatchmakingRequest& Request,
	                              const FOnMatchmakingStatusChanged& StatusCallback,
	                              const FOnMatchmakingCompleted& CompleteCallback) override;
	virtual void CancelMatchmaking(const FString& TicketId, const FOnMatchmakingCancelled& Callback) override;

	/** 매칭 브로커 주기적 틱 (0.5초 주기) */
	bool Tick(float DeltaTime);

	/** 현재 경과 시간 및 남은 시간(초) 조회 */
	int32 GetElapsedSeconds() const { return FMath::FloorToInt(ElapsedSeconds); }
	int32 GetRemainingSecondsUntilBot() const { return FMath::Max(0, FMath::CeilToInt(TimeoutSeconds - ElapsedSeconds)); }

private:
	FString GetQueueFilePath() const;
	void ReadQueueFile(TArray<TSharedPtr<FJsonValue>>& OutTickets);
	void WriteQueueFile(const TArray<TSharedPtr<FJsonValue>>& InTickets);
	void CleanExpiredTickets(TArray<TSharedPtr<FJsonValue>>& InOutTickets);
	void RemoveTicketFromFile(const FString& InTicketId);

	FWildOpponentInfo GenerateRandomBotOpponent() const;

private:
	bool bIsSearching = false;
	FWildMatchmakingRequest CurrentRequest;
	float ElapsedSeconds = 0.0f;
	float TimeoutSeconds = 10.0f;

	FOnMatchmakingStatusChanged OnStatusChanged;
	FOnMatchmakingCompleted OnCompleted;

	FTSTicker::FDelegateHandle TickerHandle;
	FCriticalSection FileLock;
};
