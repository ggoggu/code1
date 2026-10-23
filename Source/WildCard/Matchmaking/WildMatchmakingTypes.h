// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattleData.h"
#include "WildMatchmakingTypes.generated.h"

/**
 * 매칭 진행 상태 열거형
 */
UENUM(BlueprintType)
enum class EWildMatchmakingState : uint8
{
	Idle UMETA(DisplayName = "대기"),
	Searching UMETA(DisplayName = "상대 찾는 중"),
	MatchFound UMETA(DisplayName = "상대 발견"),
	Connecting UMETA(DisplayName = "서버 접속 중"),
	Cancelled UMETA(DisplayName = "매칭 취소됨"),
	Error UMETA(DisplayName = "오류 발생")
};

/**
 * 매칭된 상대방 프로필 정보
 */
USTRUCT(BlueprintType)
struct FWildOpponentInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FString PlayerName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	EWildPlayerClass PlayerClass = EWildPlayerClass::Warrior;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FName TacticalSkillID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	bool bIsBot = false;

	FWildOpponentInfo() {}
	FWildOpponentInfo(const FString& InName, EWildPlayerClass InClass, FName InSkillID, bool bInBot = false)
		: PlayerName(InName), PlayerClass(InClass), TacticalSkillID(InSkillID), bIsBot(bInBot) {}
};

/**
 * 매칭 성사 결과 정보
 */
USTRUCT(BlueprintType)
struct FWildMatchResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FString TicketId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FString MatchId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FString ServerAddress;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	bool bIsHost = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FWildDeckData MyDeckData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FWildOpponentInfo Opponent;
};

/**
 * 매칭 요청 티켓 파라미터
 */
USTRUCT(BlueprintType)
struct FWildMatchmakingRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
	FString TicketId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
	FString Username;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
	FWildDeckData SelectedDeck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
	float TimeoutSeconds = 10.0f;
};

/** C++ 내부 콜백용 비동기 델리게이트 */
DECLARE_DELEGATE_OneParam(FOnMatchmakingStatusChanged, EWildMatchmakingState /*NewState*/);
DECLARE_DELEGATE_ThreeParams(FOnMatchmakingCompleted, bool /*bSuccess*/, const FWildMatchResult& /*Result*/, const FString& /*ErrorMessage*/);
DECLARE_DELEGATE(FOnMatchmakingCancelled);

/** 블루프린트 / UMG / Slate 연동용 다이내믹 멀티캐스트 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingStateChangedSignature, EWildMatchmakingState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchmakingTimerTickSignature, int32, ElapsedSeconds, int32, RemainingSecondsUntilBot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchFoundSignature, const FWildMatchResult&, MatchResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchmakingErrorSignature, const FString&, ErrorMessage);
