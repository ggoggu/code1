// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BattleData.h"
#include "PlayerCardComponent.h" // [추가됨] 카드 전담 컴포넌트 헤더
#include "InGamePlayerState.generated.h"

class ATile;
class UPlayerStatComponent;
class UArtifactBase;
class APlayerUnit;
class UTacticalSkillBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionReadyChangedDelegate, bool, bIsReady);

UCLASS()
class WILDCARD_API AInGamePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AInGamePlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	int32 GetPlayerID() const { return PlayerID; };
	int32 GetPlayerCount() const { return PlayerCount; };

	UFUNCTION(BlueprintCallable)
	void SetPlayerCount(int32 InPlayerCount) { PlayerCount = InPlayerCount; };

	UFUNCTION(BlueprintCallable, Category = "Stats")
	UPlayerStatComponent* GetStatComponent() const { return StatComponent; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	APlayerUnit* GetPlayerUnit() const { return MyPlayerUnit; }

	UFUNCTION(BlueprintCallable, Category = "Unit")
	void SetPlayerUnit(APlayerUnit* InUnit);

	UFUNCTION()
	void OnRep_MyPlayerUnit();

	UPROPERTY(ReplicatedUsing = OnRep_MyPlayerUnit, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<APlayerUnit> MyPlayerUnit;

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	EWildPlayerClass GetPlayerClass() const { return PlayerClass; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	void SetPlayerClass(EWildPlayerClass InClass) { PlayerClass = InClass; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	TSubclassOf<APlayerUnit> GetPlayerUnitClass() const;

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	void SetPlayerUnitClass(TSubclassOf<APlayerUnit> InUnitClass) { SelectedPlayerUnitClass = InUnitClass; }

	UPROPERTY(ReplicatedUsing = OnRep_PlayerClass, BlueprintReadOnly, Category = "PlayerUnit")
	EWildPlayerClass PlayerClass = EWildPlayerClass::Common;

	UFUNCTION()
	void OnRep_PlayerClass();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "PlayerUnit")
	TSubclassOf<APlayerUnit> SelectedPlayerUnitClass;

	// --- 선택된 전술 스킬 관리 ---
	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	TSubclassOf<UTacticalSkillBase> GetSelectedTacticalSkillClass() const { return SelectedTacticalSkillClass; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	void SetSelectedTacticalSkillClass(TSubclassOf<UTacticalSkillBase> InSkillClass);

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	FName GetSelectedTacticalSkillID() const { return SelectedTacticalSkillID; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	void SetSelectedTacticalSkillID(FName InSkillID);

	UPROPERTY(ReplicatedUsing = OnRep_SelectedTacticalSkillClass, BlueprintReadOnly, Category = "PlayerUnit|TacticalSkill")
	TSubclassOf<UTacticalSkillBase> SelectedTacticalSkillClass;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedTacticalSkillID, BlueprintReadOnly, Category = "PlayerUnit|TacticalSkill")
	FName SelectedTacticalSkillID = NAME_None;

	UFUNCTION()
	void OnRep_SelectedTacticalSkillClass();

	UFUNCTION()
	void OnRep_SelectedTacticalSkillID();

	// 덱 데이터를 기반으로 직업 및 전술 스킬 일괄 설정 (서버 전용)
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void ApplyDeckData(const FWildDeckData& InDeckData);

	
private:
	int32 PlayerID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerStatComponent> StatComponent;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerCardComponent> CardComponent;

	UFUNCTION(BlueprintCallable, Category = "Components")
	UPlayerCardComponent* GetCardComponent() const { return CardComponent; }

	// =========================================================================
	// 외부 파일(PhaseComponent, InGameController 등) 오류 방지용 Wrapper 함수
	// 기존 함수명을 그대로 유지하되, 실제 실행은 CardComponent가 수행하도록 위임(Delegate)
	// =========================================================================
	inline class UCardBase* DrawCard(FName CardIDToDraw = NAME_None, int32 BaseMana = 1) { return CardComponent ? CardComponent->DrawCard(CardIDToDraw, BaseMana) : nullptr; }
	inline void Debug_GiveDummyStartingHand() { if (CardComponent) CardComponent->Debug_GiveDummyStartingHand(); }
	inline void CreateTestDeck() { if (CardComponent) CardComponent->CreateTestDeck(); }
	inline void InitDeckFromData(const FWildDeckData& InDeckData) { if (CardComponent) CardComponent->InitDeckFromData(InDeckData); }
	inline class UCardBase* FindCardInstanceByID(int32 InInstanceID) const { return CardComponent ? CardComponent->FindCardInstanceByID(InInstanceID) : nullptr; }
	inline bool IsCardInHand(int32 InInstanceID) const { return CardComponent ? CardComponent->IsCardInHand(InInstanceID) : false; }
	inline bool RemoveCardFromHand(int32 InInstanceID, ECardZone DestinationZone = ECardZone::EXILE) { return CardComponent ? CardComponent->RemoveCardFromHand(InInstanceID, DestinationZone) : false; }
	inline void SyncHandDataToDisplay() { if (CardComponent) CardComponent->SyncHandDataToDisplay(); }
	inline void ExecuteMulliganSwap() { if (CardComponent) CardComponent->ExecuteMulliganSwap(); }
	inline void ClearCachedMulliganCards() { if (CardComponent) CardComponent->CachedMulliganCards.Empty(); }
	inline const TArray<int32>& GetCachedMulliganCards() const { static TArray<int32> EmptyArray; return CardComponent ? CardComponent->CachedMulliganCards : EmptyArray; }
	// =========================================================================


	// Battle Section
public:
	UPROPERTY(Replicated)
	bool bIsGamesetReady = false;

	UPROPERTY(Replicated)
	bool bIsMulliganReady = false;

	UPROPERTY(BlueprintAssignable, Category = "PlayerState")
	FOnActionReadyChangedDelegate OnActionReadyChangedDelegate;

	UFUNCTION()
	void OnRep_IsActionReady();

	UPROPERTY(ReplicatedUsing = OnRep_IsActionReady, BlueprintReadOnly, Category = "Action")
	bool bIsActionReady = false;
	
public:
	void SetPendingMainAction(const FWildActionData& InActionData);
	const FWildActionData& GetPendingMainAction() const;
	void ClearPendingMainAction();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Action|ServerOnly")
	FWildActionData PendingMainAction;

	UPROPERTY(EditAnywhere, Category = "Cards")
	TObjectPtr<UDataTable> CardDataTable;

protected:
	UFUNCTION()
	void OnRep_PlayerCount();

public:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerCount)
	int32 PlayerCount;

	// Mulligan
public:
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_SubmitMulligan(const TArray<int32>& CardsToSwap);


	UFUNCTION(Client, Reliable)
	void Client_ReceiveCombatLogEntry(const FWildCombatLogEntry& Entry);
	
	// Artifact
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Artifact|Server")
	TObjectPtr<UArtifactBase> EquippedArtifact;

	UPROPERTY(ReplicatedUsing = OnRep_ArtifactDisplay, BlueprintReadOnly, Category = "Artifact|Client")
	FWildArtifactDisplayData ArtifactDisplay;

	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Artifact")
	void Server_SubmitArtifact(FName ArtifactRowName);

	void SyncArtifactToDisplay();
	
	UFUNCTION()
	void OnRep_ArtifactDisplay();

	// Tile Action
public:
	UPROPERTY(ReplicatedUsing = OnRep_MySpawnableTiles, BlueprintReadOnly, Category = "Tile Action")
	TArray<TObjectPtr<ATile>> MySpawnableTiles;

	UFUNCTION()
	void OnRep_MySpawnableTiles();

	void SetOldTiles(TArray<ATile*> InTiles) { OldTiles = InTiles; };
	TArray<ATile*> GetOldTiles() { return OldTiles; };
	void ClearOldTiles() { OldTiles.Empty(); };

private:
	TArray<ATile*> OldTiles;

};
