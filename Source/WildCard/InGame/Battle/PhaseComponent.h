// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "PhaseComponent.generated.h"


class AInGamePlayerState;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WILDCARD_API UPhaseComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPhaseComponent();

protected:
	virtual void BeginPlay() override;

public:
	void SetMatchState(EWildMatchState NewState);
	void SetGamePhase(EWildCardPhase NewPhase);

	// public 에서 체인 처리
	void ProcessPlayPublicAction(APlayerController* PC, const FWildActionData& ActionData);
	void ProcessPublicPass(APlayerController* PC);
	//
	
	void ProcessPlayerGamesetReady(APlayerController* PC);
	void ProcessPlayerMulliganReady(APlayerController* PC);
	void ProcessPrivateActionReady(APlayerController* PC, const FWildActionData& ActionData);
	void CheckActionPhaseEnd(bool bActionOccurred);
	void ResetCycleCounters();
	void StartPhaseCountdown(int32 DurationSeconds);
	static int32 GetActionForceCost(AInGamePlayerState* PS, const FWildActionData& ActionData);
	bool CanPlayerAct(AInGamePlayerState* PS) const;
	bool CanAnyPlayerAct() const;

private:
	FTimerHandle PhaseTimerHandle;
	int32 MulliganReadyCount = 0;
	int32 GamesetReadyCount = 0;
	int32 ActionReadyCount = 0;
	int32 ConsecutivePassCount = 0;

	void HandleTurnStart();
	void HandleTurnEnd();

	UFUNCTION()
	void OnGlobalTimerTick();

	UFUNCTION()
	void OnMatchTimerExpired();

	UFUNCTION()
	void OnPhaseTimerExpired();

	// --- 액션 유효성 검증 함수 (Validation Helpers) ---
	FWildActionData ValidatePrivateAction(AInGamePlayerState* PS, const FWildActionData& InAction);
	bool ValidateCardAction(AInGamePlayerState* PS, int32 CardInstanceID);
	bool ValidateUnitMoveAction(AInGamePlayerState* PS, const FWildActionData& ActionData);
	bool ValidateUnitAttackAction(AInGamePlayerState* PS, const FWildActionData& ActionData);

	// --- 페이즈 공통 유틸리티 함수 (Common Helpers) ---
	void ClearAllPendingActions();
	bool HasAnyPlayerCommittedMainAction() const;
	int32 GetOpponentPlayerID(int32 PlayerID) const;
	void AdvanceFromPrivateActionPhase();

private:
	static constexpr int32 GamesetTime = 20;
	static constexpr int32 MulliganTime = 20;
	static constexpr int32 TurnStartTime = 2;
	static constexpr int32 ActionPrivateTime = 20;
	static constexpr int32 ActionPublicTime = 15; 
	static constexpr int32 TurnEndTime = 2;
};
