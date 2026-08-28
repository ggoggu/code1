// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "PhaseComponent.generated.h"


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

private:
	static constexpr int32 GamesetTime = 20;
	static constexpr int32 MulliganTime = 20;
	static constexpr int32 ActionPrivateTime = 20;
	static constexpr int32 ActionPublicTime = 15; 

		
};
