// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BattleData.h"
#include "WildDeckSaveGame.generated.h"

/**
 * 로컬에 유저별 덱 목록을 영구 저장하는 세이브 게임 클래스
 */
UCLASS()
class WILDCARD_API UWildDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Key: Username (소문자 표준화 권장), Value: 유저 보유 덱 목록 */
	UPROPERTY(VisibleAnywhere, Category = "Deck")
	TMap<FString, FWildDeckList> UserDecks;
};
