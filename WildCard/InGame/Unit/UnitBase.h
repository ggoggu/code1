// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardBase.h"
#include "BattleData.h"
#include "UnitBase.generated.h"

class ATile;
struct FWildActionData;

UCLASS()
class WILDCARD_API AUnitBase : public AActor
{
	GENERATED_BODY()

public:
	AUnitBase();

	virtual void BeginPlay() override;

	// ==== getter ==== 
	int32 GetCardHealth() { return UnitCardData.Health; }
	int32 GetCardAttack() { return UnitCardData.Attack; }
	ATile* GetCurrentTile() { return CurrentTile; }

	UFUNCTION(BlueprintCallable)
	int32 GetOwnerId() { return UnitCardData.OwnerId; }

	// ----- setter ----
	void SetCardHealth(int32 InHealth);
	void SetCurrentTile(ATile* InTile) { CurrentTile = InTile; }

	// ----- Init -----
	void InitUnitCardData(FCardData InCardData, int32 InOwnerId);

	//#. Function in BP
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetWidgetScore();

	UFUNCTION(BlueprintCallable)
	FUnitCardData GetUnitCardData() { return UnitCardData; }

	FCardData GetCardData() { return CardData; }

	UFUNCTION()
	void OnRep_UnitCardDataChanged();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_UnitCardDataChanged)
	FUnitCardData UnitCardData;

	//#. 카드의 기본 정보 (DB에서 받아오는 정보)
	//#. 후에 소멸,추방 후 부활시 다시 사용될 원본 데이터
	FCardData CardData;

	FWildActionData CurrentActionData;

	UPROPERTY()
	TObjectPtr<ATile> CurrentTile;
};
