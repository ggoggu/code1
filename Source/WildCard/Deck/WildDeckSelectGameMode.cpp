// Fill out your copyright notice in the Description page of Project Settings.

#include "WildDeckSelectGameMode.h"
#include "WildDeckSelectPlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

AWildDeckSelectGameMode::AWildDeckSelectGameMode()
{
	PlayerControllerClass = AWildDeckSelectPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AWildDeckSelectGameMode::HandlePlayerMatchReady(APlayerController* PC, const FString& MatchId)
{
	if (bHasInitiatedTravel)
	{
		return;
	}

	ReadyPlayers.Add(PC);

	const int32 NumConnected = GetNumPlayers();
	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelectGameMode] 플레이어 매칭 완료 보고 수신 (%d/%d). MatchID: %s"),
		ReadyPlayers.Num(), NumConnected, *MatchId);

	// 접속 중인 모든 플레이어(2인 이상 또는 1인 봇)가 준비되면 ServerTravel 실행
	if (ReadyPlayers.Num() >= FMath::Max(1, NumConnected))
	{
		ExecuteMatchServerTravel();
	}
	else
	{
		// 첫 번째 플레이어 완료 후 3초 내에 상대 완료 보고가 오지 않더라도 안전하게 트래블 강제 실행
		if (!ForceTravelTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(
				ForceTravelTimerHandle,
				this,
				&AWildDeckSelectGameMode::ExecuteMatchServerTravel,
				3.0f,
				false
			);
		}
	}
}

void AWildDeckSelectGameMode::ExecuteMatchServerTravel()
{
	if (bHasInitiatedTravel)
	{
		return;
	}
	bHasInitiatedTravel = true;

	GetWorld()->GetTimerManager().ClearTimer(ForceTravelTimerHandle);

	FString TravelURL = NextLevelPath.ToString();
	if (!TravelURL.StartsWith(TEXT("/Game/")))
	{
		TravelURL = FString::Printf(TEXT("/Game/Level/%s"), *TravelURL);
	}

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelectGameMode] 전용 서버 ServerTravel 실행: %s"), *TravelURL);
	GetWorld()->ServerTravel(TravelURL);
}

