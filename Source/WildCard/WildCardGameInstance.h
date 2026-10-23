// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "WildCard.h"
#include "BattleData.h"
#include "WildCardGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UWildCardGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	TObjectPtr<class UDataTable> CardDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	TObjectPtr<class UDataTable> ArtifactDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	TObjectPtr<class UDataTable> TacticalSkillDataTable;

	UPROPERTY(BlueprintReadWrite, Category = "Mock DB")
	TMap<FString, FWildDeckList> MockUserDecks;

	/** 현재 선택되어 대전에 사용할 덱 */
	UPROPERTY(BlueprintReadWrite, Category = "Deck")
	FWildDeckData SelectedDeck;

	// 새 덱을 저장하거나 기존 덱을 업데이트합니다.
	UFUNCTION(BlueprintCallable, Category = "Deck")
	void SaveUserDeck(const FString& Username, const FWildDeckData& NewDeck);

	// 특정 유저의 덱 목록을 불러옵니다.
	UFUNCTION(BlueprintCallable, Category = "Deck")
	TArray<FWildDeckData> GetUserDecks(const FString& Username);

	// 특정 직업이 사용 가능한 전술 스킬 목록을 데이터 테이블에서 조회합니다.
	UFUNCTION(BlueprintCallable, Category = "Deck")
	TArray<FTacticalSkillData> GetTacticalSkillsForClass(EWildPlayerClass InClass) const;
	
};
