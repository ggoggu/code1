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

	UPROPERTY(ReplicatedUsing = OnRep_MySpawnableTiles)
	TArray<TObjectPtr<ATile>> MySpawnableTiles;
	
private:
	int32 PlayerID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPlayerStatComponent> StatComponent;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerCardComponent> CardComponent;

	// =========================================================================
	// 외부 파일(PhaseComponent, InGameController 등) 오류 방지용 Wrapper 함수
	// 기존 함수명을 그대로 유지하되, 실제 실행은 CardComponent가 수행하도록 위임(Delegate)
	// =========================================================================
	inline void DrawCard(FName CardIDToDraw, int32 BaseMana) { if (CardComponent) CardComponent->DrawCard(CardIDToDraw, BaseMana); }
	inline void Debug_GiveDummyStartingHand() { if (CardComponent) CardComponent->Debug_GiveDummyStartingHand(); }
	inline void CreateTestDeck() { if (CardComponent) CardComponent->CreateTestDeck(); }
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

	UPROPERTY(Replicated)
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
	UFUNCTION()
	void OnRep_MySpawnableTiles();

public:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerCount)
	int32 PlayerCount;

	// Mulligan
public:
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable)
	void Server_SubmitMulligan(const TArray<int32>& CardsToSwap);

	UFUNCTION(Client, Reliable)
	void Client_ReceiveStartingHand(const TArray<FWildCardDisplayData>& FinalHand);
	
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
	
};
