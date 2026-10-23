// Fill out your copyright notice in the Description page of Project Settings.


#include "WildCardGameInstance.h"
#include "WildDeckSubsystem.h"

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
			ExistingDeck = NewDeck;
			bFound = true;
			break;
		}
	}

	// 3. 기존 덱이 없다면 새 덱을 배열에 추가합니다.
	if (!bFound)
	{
		UserDeckList.Decks.Add(NewDeck);
	}

	// 4. UWildDeckSubsystem에도 동기화 저장
	if (UWildDeckSubsystem* DeckSub = GetSubsystem<UWildDeckSubsystem>())
	{
		DeckSub->SaveUserDeck(Username, NewDeck);
	}

	UE_LOG(LogTemp, Log, TEXT("[Mock DB] 유저 '%s'의 덱 '%s' 저장 완료 (직업: %d, 스킬ID: %s, 총 %d장)"), 
		*Username, *NewDeck.DeckName, static_cast<int32>(NewDeck.DeckClass), *NewDeck.SelectedTacticalSkillID.ToString(), NewDeck.CardIDs.Num());
}

TArray<FWildDeckData> UWildCardGameInstance::GetUserDecks(const FString& Username)
{
	// 1. UWildDeckSubsystem 우선 조회
	if (UWildDeckSubsystem* DeckSub = GetSubsystem<UWildDeckSubsystem>())
	{
		TArray<FWildDeckData> SubDecks = DeckSub->GetUserDecks(Username);
		if (SubDecks.Num() > 0)
		{
			return SubDecks;
		}
	}

	// 2. 레거시 MockUserDecks 폴백
	if (FWildDeckList* FoundList = MockUserDecks.Find(Username))
	{
		return FoundList->Decks;
	}
	return TArray<FWildDeckData>();
}

TArray<FTacticalSkillData> UWildCardGameInstance::GetTacticalSkillsForClass(EWildPlayerClass InClass) const
{
	TArray<FTacticalSkillData> Result;
	if (!TacticalSkillDataTable)
	{
		return Result;
	}

	static const FString ContextString(TEXT("GetTacticalSkillsForClass"));
	TArray<FTacticalSkillData*> AllRows;
	TacticalSkillDataTable->GetAllRows<FTacticalSkillData>(ContextString, AllRows);

	for (const FTacticalSkillData* Row : AllRows)
	{
		if (Row && (Row->RequiredClass == EWildPlayerClass::Common || Row->RequiredClass == InClass))
		{
			Result.Add(*Row);
		}
	}

	return Result;
}