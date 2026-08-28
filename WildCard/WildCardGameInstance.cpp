// Fill out your copyright notice in the Description page of Project Settings.


#include "WildCardGameInstance.h"

void UWildCardGameInstance::SaveUserDeck(const FString& Username, const FWildDeckData& NewDeck)
{
	// 1. 유저의 덱 리스트(래퍼 구조체)를 가져오거나 새로 만듭니다.
	FWildDeckList& UserDeckList = MockUserDecks.FindOrAdd(Username);

	// 2. 래퍼 구조체 안의 배열(Decks)을 순회합니다.
	bool bFound = false;
	for (FWildDeckData& ExistingDeck : UserDeckList.Decks)
	{
		if (ExistingDeck.DeckName == NewDeck.DeckName)
		{
			ExistingDeck.CardIDs = NewDeck.CardIDs;
			bFound = true;
			break;
		}
	}

	// 3. 기존 덱이 없다면 새 덱을 배열에 추가합니다.
	if (!bFound)
	{
		UserDeckList.Decks.Add(NewDeck);
	}

	UE_LOG(LogTemp, Log, TEXT("[Mock DB] 유저 '%s'의 덱 '%s' 저장 완료 (총 %d장)"), *Username, *NewDeck.DeckName, NewDeck.CardIDs.Num());
}

TArray<FWildDeckData> UWildCardGameInstance::GetUserDecks(const FString& Username)
{
	// 래퍼 구조체를 찾은 뒤, 그 안의 Decks 배열만 뽑아서 반환합니다.
	if (FWildDeckList* FoundList = MockUserDecks.Find(Username))
	{
		return FoundList->Decks;
	}
	return TArray<FWildDeckData>();
}