// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "InGameController.h"
#include "InGameState.generated.h"

enum class EWildCardPhase : uint8;
enum class EWildMatchState : uint8;
class ATileMapMaker;
class AUnitBase;
class UChainComponent;
class ATileMap;
class ATile;
class AInGameController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerChangedDelegate, int32, RemainTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnStartedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnEndedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangeDelegate, EWildCardPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchStateChangeDelegate, EWildMatchState, NewMatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitDamagedSignature, AUnitBase*, DamagedUnit, int32, DamageAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDestroyedSignature, AUnitBase*, DestroyedUnit);

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

private:
	UPROPERTY(ReplicatedUsing = OnRep_MapMaker)
	TObjectPtr<ATileMapMaker> MapMaker;

	UPROPERTY()
	TObjectPtr<ATileMap> Map;

	//#. Test용 array
	UPROPERTY(ReplicatedUsing = OnRep_UnitArrayChanged)
	TArray<TWeakObjectPtr<AUnitBase>> UnitsArray;

	UPROPERTY(ReplicatedUsing = OnRep_TileArrayChanged)
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

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMatchState, BlueprintReadOnly, Category = "Match")
	EWildMatchState CurrentMatchState;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly, Category = "Phase")
	EWildCardPhase CurrentPhase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UChainComponent> ChainComponent;

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
