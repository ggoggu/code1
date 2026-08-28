// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "PlayerCardComponent.generated.h"

class ACardBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHandCountChangedDelegate, int32, NewHandCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMyHandUpdatedDelegate, const TArray<FWildCardDisplayData>&, HandData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckCountChangedDelegate, int32, NewDeckCount);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WILDCARD_API UPlayerCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlayerCardComponent();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Events") FOnMyHandUpdatedDelegate OnMyHandUpdatedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnHandCountChangedDelegate OnHandCountChangedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Events") FOnDeckCountChangedDelegate OnDeckCountChangedDelegate;

	// 카드 로직 함수들
	void DrawCard(FName CardIDToDraw, int32 BaseMana);
	void Debug_GiveDummyStartingHand();
	void CreateTestDeck();
	void ShuffleDeck();
	ACardBase* GenerateCardInstance(FName CardRowName);
	ACardBase* FindCardInstanceByID(int32 InInstanceID) const;
	void SyncHandDataToDisplay();
	void ExecuteMulliganSwap();

	// 카드 구역(Zone) 데이터
	UPROPERTY(VisibleAnywhere, Category = "Cards|Server") TArray<TObjectPtr<ACardBase>> ServerDeck;
	UPROPERTY(VisibleAnywhere, Category = "Cards|Server") TArray<TObjectPtr<ACardBase>> ServerHand;
	UPROPERTY(VisibleAnywhere, Category = "Cards|Server") TArray<TObjectPtr<ACardBase>> ServerObliterate;
	UPROPERTY(VisibleAnywhere, Category = "Cards|Server") TArray<TObjectPtr<ACardBase>> ServerExile;

	UPROPERTY()
	FRandomStream DeckRandomStream;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHandDisplay, BlueprintReadOnly, Category = "Cards|Client")
	TArray<FWildCardDisplayData> CurrentHandDisplay;

	UPROPERTY(ReplicatedUsing = OnRep_HandCount, BlueprintReadOnly, Category = "Cards|Count") int32 HandCount;
	UPROPERTY(ReplicatedUsing = OnRep_DeckCount, BlueprintReadOnly, Category = "Cards|Count") int32 DeckCount;

	TArray<int32> CachedMulliganCards;

protected:
	UFUNCTION() void OnRep_CurrentHandDisplay();
	UFUNCTION() void OnRep_HandCount();
	UFUNCTION() void OnRep_DeckCount();
};
