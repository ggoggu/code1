// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WildMatchmakingTypes.h"

/**
 * 매칭 프로바이더 인터페이스
 * 로컬 파일 브로커(에디터 2클라이언트 & 봇 폴백), EOS SDK Sessions, AWS GameLift FlexMatch를 추상화합니다.
 */
class WILDCARD_API IWildMatchmakingProvider
{
public:
	virtual ~IWildMatchmakingProvider() = default;

	/** 프로바이더 초기화 */
	virtual void Initialize() = 0;

	/** 매칭 탐색 시작 */
	virtual void StartMatchmaking(const FWildMatchmakingRequest& Request,
	                              const FOnMatchmakingStatusChanged& StatusCallback,
	                              const FOnMatchmakingCompleted& CompleteCallback) = 0;

	/** 매칭 취소 요청 */
	virtual void CancelMatchmaking(const FString& TicketId, const FOnMatchmakingCancelled& Callback) = 0;
};
