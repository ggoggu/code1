// Fill out your copyright notice in the Description page of Project Settings.

#include "WildMatchmakingSubsystem.h"
#include "WildLocalMatchmakingProvider.h"
#include "WildAuthSubsystem.h"
#include "WildCardGameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWildMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 기본 로컬 프로바이더 생성 및 초기화
	SetProvider(MakeShared<FWildLocalMatchmakingProvider>());

	UE_LOG(LogTemp, Log, TEXT("[WildMatchmakingSubsystem] Initialized with LocalMatchmakingProvider."));
}

void UWildMatchmakingSubsystem::Deinitialize()
{
	CancelMatchmaking();
	ActiveProvider.Reset();
	Super::Deinitialize();
}

void UWildMatchmakingSubsystem::SetProvider(TSharedPtr<IWildMatchmakingProvider> InProvider)
{
	if (ActiveProvider.IsValid() && IsSearching())
	{
		CancelMatchmaking();
	}

	ActiveProvider = InProvider;
	if (ActiveProvider.IsValid())
	{
		ActiveProvider->Initialize();
	}
}

void UWildMatchmakingSubsystem::StartMatchmaking(const FWildDeckData& InDeck, float TimeoutSeconds)
{
	if (!ActiveProvider.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[WildMatchmakingSubsystem] ActiveProvider가 유효하지 않습니다."));
		OnMatchmakingError.Broadcast(TEXT("매칭 프로바이더가 설정되지 않았습니다."));
		return;
	}

	if (!InDeck.IsValidDeck())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildMatchmakingSubsystem] 선택된 덱이 유효하지 않아 매칭을 시작할 수 없습니다. (현재: %d장)"), InDeck.CardIDs.Num());
		OnMatchmakingError.Broadcast(TEXT("선택된 덱의 카드 수가 유효하지 않습니다."));
		return;
	}

	// 유저 이름 식별
	FString Username = TEXT("Player");
	if (UWildAuthSubsystem* AuthSub = GetGameInstance()->GetSubsystem<UWildAuthSubsystem>())
	{
		if (AuthSub->IsLoggedIn())
		{
			Username = AuthSub->GetCurrentSession().Username;
		}
		else
		{
			// PIE 2클라이언트 테스트 시 미로그인 상태에서도 서로 구별되도록 랜덤 번호 부착
			Username = FString::Printf(TEXT("Player_%04d"), FMath::RandRange(1000, 9999));
		}
	}

	ActiveTicketId = FGuid::NewGuid().ToString();

	FWildMatchmakingRequest Request;
	Request.TicketId = ActiveTicketId;
	Request.Username = Username;
	Request.SelectedDeck = InDeck;
	Request.TimeoutSeconds = TimeoutSeconds;

	CurrentElapsedSeconds = 0;
	CurrentRemainingSeconds = FMath::CeilToInt(TimeoutSeconds);

	SetState(EWildMatchmakingState::Searching);

	// 1초 단위 타이머 등록 (UI 시간 갱신용)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SecondTimerHandle, this, &UWildMatchmakingSubsystem::OnSecondTick, 1.0f, true);
	}

	ActiveProvider->StartMatchmaking(
		Request,
		FOnMatchmakingStatusChanged::CreateUObject(this, &UWildMatchmakingSubsystem::HandleStatusChanged),
		FOnMatchmakingCompleted::CreateUObject(this, &UWildMatchmakingSubsystem::HandleMatchCompleted)
	);

	UE_LOG(LogTemp, Log, TEXT("[WildMatchmakingSubsystem] 매칭 탐색 시작 (User: %s, Deck: %s, Timeout: %.1f)"),
		*Username, *InDeck.DeckName, TimeoutSeconds);
}

void UWildMatchmakingSubsystem::CancelMatchmaking()
{
	if (CurrentState == EWildMatchmakingState::Idle || CurrentState == EWildMatchmakingState::Cancelled)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SecondTimerHandle);
	}

	if (ActiveProvider.IsValid() && !ActiveTicketId.IsEmpty())
	{
		ActiveProvider->CancelMatchmaking(
			ActiveTicketId,
			FOnMatchmakingCancelled::CreateUObject(this, &UWildMatchmakingSubsystem::HandleMatchCancelled)
		);
	}
	else
	{
		HandleMatchCancelled();
	}
}

void UWildMatchmakingSubsystem::SetState(EWildMatchmakingState NewState)
{
	if (CurrentState != NewState)
	{
		CurrentState = NewState;
		OnStateChanged.Broadcast(CurrentState);
	}
}

void UWildMatchmakingSubsystem::HandleStatusChanged(EWildMatchmakingState InState)
{
	SetState(InState);
}

void UWildMatchmakingSubsystem::HandleMatchCompleted(bool bSuccess, const FWildMatchResult& Result, const FString& ErrorMessage)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SecondTimerHandle);
	}

	if (bSuccess)
	{
		LastMatchResult = Result;

		// GameInstance의 활성 덱과도 동기화
		if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
		{
			GI->SelectedDeck = Result.MyDeckData;
		}

		SetState(EWildMatchmakingState::MatchFound);
		OnMatchFound.Broadcast(LastMatchResult);

		UE_LOG(LogTemp, Log, TEXT("[WildMatchmakingSubsystem] 매칭 성사 완료! MatchID: %s, 상대: %s (봇: %d), 접속주소: %s, 호스트: %d"),
			*Result.MatchId, *Result.Opponent.PlayerName, Result.Opponent.bIsBot, *Result.ServerAddress, Result.bIsHost);
	}
	else
	{
		SetState(EWildMatchmakingState::Error);
		OnMatchmakingError.Broadcast(ErrorMessage);
		UE_LOG(LogTemp, Warning, TEXT("[WildMatchmakingSubsystem] 매칭 실패: %s"), *ErrorMessage);
	}
}

void UWildMatchmakingSubsystem::HandleMatchCancelled()
{
	ActiveTicketId.Empty();
	SetState(EWildMatchmakingState::Cancelled);
	SetState(EWildMatchmakingState::Idle);
}

void UWildMatchmakingSubsystem::OnSecondTick()
{
	if (CurrentState != EWildMatchmakingState::Searching)
	{
		return;
	}

	CurrentElapsedSeconds++;
	CurrentRemainingSeconds = FMath::Max(0, CurrentRemainingSeconds - 1);

	OnTimerTick.Broadcast(CurrentElapsedSeconds, CurrentRemainingSeconds);
}
