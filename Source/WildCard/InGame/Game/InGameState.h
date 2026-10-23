// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "InGameController.h"
#include "BattleData.h"
#include "InGameState.generated.h"

enum class EWildCardPhase : uint8;
enum class EWildMatchState : uint8;
enum class EWildTimingNoticeType : uint8;
class ATileMapMaker;
class AUnitBase;
class UChainComponent;
class ATileMap;
class ATile;
class AInGameController;
class UWildCombatLogComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerChangedDelegate, int32, RemainTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangeDelegate, EWildCardPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChangeDelegate, EWildMatchState, NewMatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitDamagedSignature, AUnitBase*, DamagedUnit, int32, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDestroyedSignature, AUnitBase*, DestroyedUnit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChainPromptChangedDelegate, int32, NewPromptPlayerID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEndedDelegate, int32, WinnerPlayerID, int32, DefeatedPlayerID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisualCueReceivedDelegate, const FWildVisualCueData&, CueData);

UCLASS()
class WILDCARD_API AInGameState : public AGameState
{
	GENERATED_BODY()

public:
	AInGameState();
	virtual void BeginPlay() override;

	void InitMap();
	void CreateMapTile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void HandleUnitSpawned(TWeakObjectPtr<AUnitBase> InSpawnedUnit, TWeakObjectPtr<ATile> InSpawnedTile);
	void HandleUnitDestroyed(TWeakObjectPtr<AUnitBase> InDestroyedUnit, TWeakObjectPtr<ATile> InDestroyedTile);
	void HandleUnitMoved(TWeakObjectPtr<ATile> OldTile, TWeakObjectPtr<ATile> NewTile);

private:
	UFUNCTION()
	void OnRep_MapMaker();

public:
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<ATileMapMaker> MapMakerClass;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FindCanMoveUnitTiles(ATile* InTile, AInGameController* InPlayer);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FindCanCombatUnitTiles(ATile* InTile, AInGameController* InPlayer);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FindCanEffectableUnitTiles(ATile* InTile, AInGameController* InPlayer);

	// 전체 클라이언트에 타이밍 공지 연출 브로드캐스트 RPC (코스메틱 연출용)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_NotifyTiming(EWildTimingNoticeType NoticeType, const FText& CustomMessage = FText::GetEmpty(), float InDuration = -1.0f);

	// 전체 클라이언트에 범용 시각/UI 연출 브로드캐스트 RPC (코스메틱 연출용)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_BroadcastVisualCue(const FWildVisualCueData& CueData);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Unit")
	void RegisterSpawnedUnit(AUnitBase* InSpawnedUnit, ATile* InSpawnedTile);

private:
	UPROPERTY()
	TObjectPtr<ATileMapMaker> MapMaker;

	UPROPERTY(Replicated)
	TObjectPtr<ATileMap> Map;

	//#. Test용 array (내부 관리용, TWeakObjectPtr는 엔진 네트워크 복제 미지원)
	UPROPERTY()
	TArray<TWeakObjectPtr<AUnitBase>> UnitsArray;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> TilesArray;
	
public:
	// Event Bus
	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnTurnStartedDelegate OnTurnStartedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnTurnEndedDelegate OnTurnEndedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnUnitDestroyedSignature OnUnitDestroyedEvent; 
	
	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnPhaseChangeDelegate OnPhaseChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnTimerChangedDelegate OnTimerChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnMatchStateChangeDelegate OnMatchStateChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnChainPromptChangedDelegate OnChainPromptChangedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnMatchEndedDelegate OnMatchEndedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Event Bus")
	FOnVisualCueReceivedDelegate OnVisualCueReceivedDelegate;

	UPROPERTY(ReplicatedUsing = OnRep_WinningPlayerID, BlueprintReadOnly, Category = "Match")
	int32 WinningPlayerID;

	UFUNCTION()
	void OnRep_WinningPlayerID();

	int32 GetWinningPlayerID() const { return WinningPlayerID; }
	void SetWinningPlayerID(int32 InWinnerID, int32 InDefeatedID = -1);
	//TEST//
	ATileMapMaker* GetMapMaker() const { return MapMaker; }
	ATileMap* GetMap() const { return Map; }
	//TEST//
	
	/* 아직 사용 x */
	UPROPERTY(VisibleAnywhere, Category = "Board")
	TArray<TObjectPtr<AUnitBase>> ActiveUnitsOnBoard;
	
	void RegisterUnit(AUnitBase* NewUnit);
	void UnregisterUnit(AUnitBase* DeadUnit);
	/*			*/

	TArray<ATile*> SelectCanSpawnUnitTiles(TWeakObjectPtr<ATile> InSpawnedTile, int32 InDistance);

	TArray<TWeakObjectPtr<ATile>> GetTilesArray() { return TilesArray; }

	EWildCardPhase GetCurrentPhase() const { return CurrentPhase; }
	void SetCurrentPhase(EWildCardPhase NewPhase);

	int32 GetPhaseRemainTime() const { return PhaseRemainTime; }
	void SetPhaseRemainTime(int32 NewTime);

	int32 GetCurrentPriorityPlayerID() const { return CurrentPriorityPlayerID; }
	void SetCurrentPriorityPlayerID(int32 NewPlayerID);

	int32 GetCurrentChainPromptID() const { return CurrentChainPromptID; }
	void SetCurrentChainPromptID(int32 InPlayerID);

	EWildMatchState GetCurrentMatchState() const { return CurrentMatchState; }
	void SetCurrentMatchState(EWildMatchState NewState);

	UWildCombatLogComponent* GetCombatLogComponent() const { return CombatLogComponent; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMatchState, BlueprintReadOnly, Category = "Match")
	EWildMatchState CurrentMatchState;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly, Category = "Phase")
	EWildCardPhase CurrentPhase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UChainComponent> ChainComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWildCombatLogComponent> CombatLogComponent;

	UPROPERTY(ReplicatedUsing = OnRep_PriorityPlayerChanged, BlueprintReadOnly, Category = "Phase")
	int32 CurrentPriorityPlayerID;

	UPROPERTY(ReplicatedUsing = OnRep_PhaseRemainTime, BlueprintReadOnly, Category = "Phase")
	int32 PhaseRemainTime;
	
	UPROPERTY(ReplicatedUsing = OnRep_ChainPromptPlayerChanged, BlueprintReadOnly, Category = "WildCard|State")
	int32 CurrentChainPromptID;

	UFUNCTION() void OnRep_CurrentMatchState();
	UFUNCTION() void OnRep_CurrentPhase();
	UFUNCTION() void OnRep_PriorityPlayerChanged();
	UFUNCTION() void OnRep_PhaseRemainTime();
	UFUNCTION() void OnRep_ChainPromptPlayerChanged();

	//#. Test용, UnitArray 변경 감지용
	UFUNCTION()
	void OnRep_UnitArrayChanged();

	UFUNCTION()
	void OnRep_TileArrayChanged();
};
