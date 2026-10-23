// Fill out your copyright notice in the Description page of Project Settings.

#include "WildDeckSubsystem.h"
#include "WildDeckSaveGame.h"
#include "WildCardGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"

const FString UWildDeckSubsystem::SaveSlotName = TEXT("WildCard_DeckData");

void UWildDeckSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadFromDisk();

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] Initialized. Cached users count: %d"), UserDecksCache.Num());
}

void UWildDeckSubsystem::Deinitialize()
{
	SaveToDisk();
	Super::Deinitialize();
}

TArray<FWildDeckData> UWildDeckSubsystem::GetUserDecks(const FString& InUsername)
{
	const FString Key = InUsername.ToLower().TrimStartAndEnd();
	if (FWildDeckList* FoundList = UserDecksCache.Find(Key))
	{
		return FoundList->Decks;
	}
	return TArray<FWildDeckData>();
}

bool UWildDeckSubsystem::SaveUserDeck(const FString& InUsername, const FWildDeckData& InDeck)
{
	if (InUsername.TrimStartAndEnd().IsEmpty() || InDeck.DeckName.TrimStartAndEnd().IsEmpty())
	{
		return false;
	}

	const FString Key = InUsername.ToLower().TrimStartAndEnd();
	FWildDeckList& UserList = UserDecksCache.FindOrAdd(Key);

	bool bUpdated = false;
	for (FWildDeckData& ExistingDeck : UserList.Decks)
	{
		if (ExistingDeck.DeckName == InDeck.DeckName)
		{
			ExistingDeck = InDeck;
			bUpdated = true;
			break;
		}
	}

	if (!bUpdated)
	{
		UserList.Decks.Add(InDeck);
	}

	SaveToDisk();
	OnDeckListChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] 덱 '%s' 저장 완료 (유저: %s, 직업: %d, 총 %d장)"),
		*InDeck.DeckName, *InUsername, static_cast<int32>(InDeck.DeckClass), InDeck.CardIDs.Num());

	return true;
}

bool UWildDeckSubsystem::DeleteUserDeck(const FString& InUsername, const FString& InDeckName)
{
	const FString Key = InUsername.ToLower().TrimStartAndEnd();
	if (FWildDeckList* FoundList = UserDecksCache.Find(Key))
	{
		const int32 RemovedCount = FoundList->Decks.RemoveAll([&InDeckName](const FWildDeckData& Item)
		{
			return Item.DeckName == InDeckName;
		});

		if (RemovedCount > 0)
		{
			// 만약 삭제된 덱이 현재 선택된 덱이었다면 선택 해제
			if (bHasSelectedDeck && SelectedDeck.DeckName == InDeckName)
			{
				ClearSelectedDeck();
			}

			SaveToDisk();
			OnDeckListChanged.Broadcast();
			return true;
		}
	}
	return false;
}

void UWildDeckSubsystem::SetSelectedDeck(const FWildDeckData& InDeck)
{
	SelectedDeck = InDeck;
	bHasSelectedDeck = true;

	// GameInstance의 레거시 MockUserDecks/SelectedDeck과도 동기화
	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
	{
		GI->SelectedDeck = InDeck;
	}

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] 활성 덱 선택됨: '%s' (직업: %d, 카드 수: %d, 스킬ID: %s)"),
		*SelectedDeck.DeckName, static_cast<int32>(SelectedDeck.DeckClass), SelectedDeck.CardIDs.Num(), *SelectedDeck.SelectedTacticalSkillID.ToString());

	OnDeckSelected.Broadcast(SelectedDeck);
}

void UWildDeckSubsystem::ClearSelectedDeck()
{
	SelectedDeck = FWildDeckData();
	bHasSelectedDeck = false;

	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
	{
		GI->SelectedDeck = SelectedDeck;
	}
}

static FName FindCardRowName(UDataTable* DataTable, const FString& TargetName, FName FallbackRowName)
{
	if (!DataTable)
	{
		return FallbackRowName;
	}

	const FString CleanTarget = TargetName.Replace(TEXT(" "), TEXT("")).ToLower();
	static const FString ContextStr(TEXT("FindCardRowName"));

	for (const FName& RowName : DataTable->GetRowNames())
	{
		if (const FCardData* Row = DataTable->FindRow<FCardData>(RowName, ContextStr))
		{
			const FString CleanCardName = Row->CardName.Replace(TEXT(" "), TEXT("")).ToLower();
			if (CleanCardName == CleanTarget)
			{
				return RowName;
			}
			if (CleanTarget == TEXT("수탉") && CleanCardName == TEXT("닭"))
			{
				return RowName;
			}
			if (CleanTarget == TEXT("닭") && CleanCardName == TEXT("수탉"))
			{
				return RowName;
			}
		}
	}

	return FallbackRowName;
}

bool UWildDeckSubsystem::ValidateDeck(const FWildDeckData& InDeck, FText& OutErrorMessage) const
{
	if (!InDeck.IsValidDeck())
	{
		OutErrorMessage = FText::Format(
			FText::FromString(TEXT("덱은 26장 또는 30장의 카드로 구성되어야 합니다. (현재: {0}장)")),
			FText::AsNumber(InDeck.CardIDs.Num())
		);
		return false;
	}

	// 카드 중복 장수 검사 (동일 카드 최대 2장)
	TMap<FName, int32> CardCounts;
	for (const FName& CardID : InDeck.CardIDs)
	{
		int32& Count = CardCounts.FindOrAdd(CardID);
		Count++;
		if (Count > 2)
		{
			OutErrorMessage = FText::Format(
				FText::FromString(TEXT("동일한 카드는 덱에 최대 2장까지만 포함할 수 있습니다. (초과 카드: {0})")),
				FText::FromName(CardID)
			);
			return false;
		}
	}

	OutErrorMessage = FText::GetEmpty();
	return true;
}

void UWildDeckSubsystem::EnsureStarterDecks(const FString& InUsername)
{
	const FString Key = InUsername.ToLower().TrimStartAndEnd();
	FWildDeckList& UserList = UserDecksCache.FindOrAdd(Key);

	// 마법사 기본 덱 생성
	FWildDeckData MageStarterDeck = CreateStarterDeck(EWildPlayerClass::Mage, TEXT("기본 마법사덱"));

	// 이미 덱 목록에 마법사 덱이 있다면 최신 13종x2장(총 26장) 구성으로 갱신
	bool bMageDeckFound = false;
	for (FWildDeckData& Deck : UserList.Decks)
	{
		if (Deck.DeckName == TEXT("기본 마법사덱") || (Deck.DeckClass == EWildPlayerClass::Mage && Deck.DeckName.Contains(TEXT("기본"))))
		{
			Deck = MageStarterDeck;
			bMageDeckFound = true;
			break;
		}
	}

	if (!bMageDeckFound)
	{
		UserList.Decks.Insert(MageStarterDeck, 0); // 마법사덱을 가장 먼저 노출
	}

	// 전사덱/도사덱이 없다면 추가
	bool bWarriorDeckFound = false;
	bool bTaoistDeckFound = false;
	for (const FWildDeckData& Deck : UserList.Decks)
	{
		if (Deck.DeckClass == EWildPlayerClass::Warrior) bWarriorDeckFound = true;
		if (Deck.DeckClass == EWildPlayerClass::Taoist) bTaoistDeckFound = true;
	}

	if (!bWarriorDeckFound)
	{
		UserList.Decks.Add(CreateStarterDeck(EWildPlayerClass::Warrior, TEXT("기본 전사덱")));
	}
	if (!bTaoistDeckFound)
	{
		UserList.Decks.Add(CreateStarterDeck(EWildPlayerClass::Taoist, TEXT("기본 도사덱")));
	}

	SaveToDisk();
	OnDeckListChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] 유저 '%s'에게 기본 스타터 덱 갱신 완료 (마법사 덱 26장 구성 적용)"), *InUsername);
}

FWildDeckData UWildDeckSubsystem::CreateStarterDeck(EWildPlayerClass InClass, const FString& InDeckName)
{
	FWildDeckData NewDeck(InDeckName, TArray<FName>(), InClass);

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance());
	UDataTable* CardDT = GI ? GI->CardDataTable.Get() : nullptr;
	if (!CardDT)
	{
		CardDT = LoadObject<UDataTable>(nullptr, TEXT("/Game/InGame/Data/DT_InGameCardData.DT_InGameCardData"));
		if (GI && CardDT)
		{
			GI->CardDataTable = CardDT;
		}
	}

	if (InClass == EWildPlayerClass::Mage)
	{
		// 마법사 기본 덱: 사용자가 지정한 13종 카드를 각 2장씩 (총 26장) 구성
		struct FMageDeckCardSpec
		{
			const TCHAR* CardName;
			const TCHAR* FallbackRow;
		};

		const FMageDeckCardSpec TargetMageCards[] = {
			{ TEXT("거미"), TEXT("NewRow_0") },
			{ TEXT("딱정벌레"), TEXT("NewRow_2") },
			{ TEXT("다람쥐"), TEXT("NewRow_6") },
			{ TEXT("수탉"), TEXT("NewRow_9") },
			{ TEXT("설원의 저격수 시모"), TEXT("NewRow_11") },
			{ TEXT("수련"), TEXT("NewRow_12") },
			{ TEXT("대마법사 멀린"), TEXT("NewRow_14") },
			{ TEXT("통제할 수 없는 무장 여포"), TEXT("NewRow_15") },
			{ TEXT("로켓 발사체"), TEXT("NewRow_16") },
			{ TEXT("탈피"), TEXT("NewRow_17") },
			{ TEXT("살해 저주"), TEXT("NewRow_19") },
			{ TEXT("운기조식"), TEXT("NewRow_21") },
			{ TEXT("가속"), TEXT("NewRow_22") }
		};

		for (const FMageDeckCardSpec& Spec : TargetMageCards)
		{
			const FName CardRow = FindCardRowName(CardDT, Spec.CardName, FName(Spec.FallbackRow));
			NewDeck.CardIDs.Add(CardRow);
			NewDeck.CardIDs.Add(CardRow);
		}
	}
	else if (CardDT)
	{
		static const FString ContextStr(TEXT("CreateStarterDeck"));
		TArray<FName> RowNames = CardDT->GetRowNames();

		TArray<FName> ValidCards;
		for (const FName& RowName : RowNames)
		{
			if (FCardData* Row = CardDT->FindRow<FCardData>(RowName, ContextStr))
			{
				if (Row->bIsCollectible && (Row->CardClass == InClass || Row->CardClass == EWildPlayerClass::Common))
				{
					ValidCards.Add(RowName);
				}
			}
		}

		// 수집된 유효 카드들을 최대 2장씩 추가하여 26~30장 채우기
		for (const FName& CardName : ValidCards)
		{
			if (NewDeck.CardIDs.Num() >= 26) break;
			NewDeck.CardIDs.Add(CardName);
			if (NewDeck.CardIDs.Num() < 26)
			{
				NewDeck.CardIDs.Add(CardName);
			}
		}

		// 여전히 카드가 부족한 경우 가용한 카드를 순환 보충
		if (ValidCards.Num() > 0)
		{
			int32 LoopIdx = 0;
			while (NewDeck.CardIDs.Num() < 26)
			{
				NewDeck.CardIDs.Add(ValidCards[LoopIdx % ValidCards.Num()]);
				LoopIdx++;
			}
		}
		else
		{
			for (int32 i = 0; i < 26; ++i)
			{
				const FName FallbackRow = (RowNames.Num() > 0) ? RowNames[i % RowNames.Num()] : FName(*FString::Printf(TEXT("Card_%d"), i + 1));
				NewDeck.CardIDs.Add(FallbackRow);
			}
		}
	}
	else
	{
		for (int32 i = 0; i < 26; ++i)
		{
			NewDeck.CardIDs.Add(FName(*FString::Printf(TEXT("Card_%d"), i + 1)));
		}
	}

	// 기본 전술 스킬 설정
	if (GI && GI->TacticalSkillDataTable)
	{
		TArray<FTacticalSkillData> Skills = GI->GetTacticalSkillsForClass(InClass);
		if (Skills.Num() > 0)
		{
			NewDeck.SelectedTacticalSkillID = Skills[0].SkillID;
			NewDeck.SelectedTacticalSkillClass = Skills[0].SkillClass;
		}
	}

	return NewDeck;
}

void UWildDeckSubsystem::SaveToDisk()
{
	UWildDeckSaveGame* SaveGameInstance = Cast<UWildDeckSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWildDeckSaveGame::StaticClass()));

	if (SaveGameInstance)
	{
		SaveGameInstance->UserDecks = UserDecksCache;
		const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveSlotName, 0);
		UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] SaveToDisk: %s"), bSuccess ? TEXT("성공") : TEXT("실패"));
	}
}

void UWildDeckSubsystem::LoadFromDisk()
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		if (UWildDeckSaveGame* LoadedGame = Cast<UWildDeckSaveGame>(
			UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
		{
			UserDecksCache = LoadedGame->UserDecks;
			UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] LoadFromDisk 성공. 유저 수: %d"), UserDecksCache.Num());
			return;
		}
	}

	UserDecksCache.Empty();
	UE_LOG(LogTemp, Log, TEXT("[WildDeckSubsystem] SaveGame이 없어 새로운 캐시로 시작합니다."));
}
