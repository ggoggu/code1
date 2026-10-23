// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleData.h"
#include "WildDeckSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeckSelectedSignature, const FWildDeckData&, SelectedDeck);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeckListChangedSignature);

/**
 * 덱 관리 서브시스템 (GameInstance 전역)
 * 유저별 덱 CRUD, 로컬 세이브게임 저장/불러오기, 스타터 덱 자동 시딩, 현재 선택된 덱 캐싱 관리
 */
UCLASS()
class WILDCARD_API UWildDeckSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 특정 유저의 덱 목록 반환 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	TArray<FWildDeckData> GetUserDecks(const FString& InUsername);

	/** 특정 유저의 덱을 저장하거나 기존 덱을 업데이트 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	bool SaveUserDeck(const FString& InUsername, const FWildDeckData& InDeck);

	/** 특정 유저의 특정 덱 삭제 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	bool DeleteUserDeck(const FString& InUsername, const FString& InDeckName);

	/** 현재 플레이에 사용할 활성 덱 선택 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void SetSelectedDeck(const FWildDeckData& InDeck);

	/** 현재 선택된 덱 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	const FWildDeckData& GetSelectedDeck() const { return SelectedDeck; }

	/** 유효한 덱이 선택되어 있는지 여부 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	bool HasSelectedDeck() const { return bHasSelectedDeck && SelectedDeck.CardIDs.Num() > 0; }

	/** 선택된 덱 초기화 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void ClearSelectedDeck();

	/** 덱 유효성 검사 (30장 채워졌는지, 직업/전술스킬 정합성 검사) */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	bool ValidateDeck(const FWildDeckData& InDeck, FText& OutErrorMessage) const;

	/** 유저의 덱이 비어있는 경우 기본 스타터 덱(전사, 마법사, 도사) 자동 생성 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void EnsureStarterDecks(const FString& InUsername);

	/** 직업별 기본 스타터 덱 1종 생성 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	FWildDeckData CreateStarterDeck(EWildPlayerClass InClass, const FString& InDeckName);

	/** 디스크 파일(SaveGame)로 저장 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void SaveToDisk();

	/** 디스크 파일(SaveGame)에서 불러오기 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void LoadFromDisk();

public:
	/** 덱 선택 완료 이벤트 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Deck")
	FOnDeckSelectedSignature OnDeckSelected;

	/** 덱 목록 변경 이벤트 알림 (추가, 수정, 삭제) */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Deck")
	FOnDeckListChangedSignature OnDeckListChanged;

protected:
	/** 유저별 덱 목록 메모리 캐시 (Key: Username) */
	UPROPERTY()
	TMap<FString, FWildDeckList> UserDecksCache;

	/** 현재 매칭/대전에 사용할 선택된 덱 */
	UPROPERTY()
	FWildDeckData SelectedDeck;

	/** 덱이 선택되었는지 여부 플래그 */
	UPROPERTY()
	bool bHasSelectedDeck = false;

	/** SaveGame 슬롯 이름 */
	static const FString SaveSlotName;
};
